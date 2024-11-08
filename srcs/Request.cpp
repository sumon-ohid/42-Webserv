#include <cstddef>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>
#include <sys/types.h>
#include <vector>

#include "../includes/Request.hpp"
#include "../includes/Client.hpp"
#include "../includes/GetMethod.hpp"
#include "../includes/main.hpp"
#include "../includes/Response.hpp"
#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"
#include "../includes/PostMethod.hpp"
#include "../includes/DeleteMethod.hpp"
#include "../includes/Helper.hpp"

Request::Request() {
	_type = -1;
	_firstLineChecked = false;
	_headerChecked = false;
	_readingFinished = false;
	_isChunked = false;
	_method = NULL;
	_response = new Response();
	_contentLength = 0;
	_contentRead = 0;
	_servConf = NULL;
}

Request::Request(const Request& other) {
	_type = other._type;
	_firstLineChecked = other._firstLineChecked;
	_headerChecked = other._headerChecked;
	_readingFinished = other._readingFinished;
	_isChunked = other._isChunked;
	if (other._method)
		_method = other._method->clone();
	else
	 	_method = NULL;
	if (other._response)
		_response = other._response->clone();
	else
	 	_response = NULL;
	_headerMap = other._headerMap;
	_contentLength = other._contentLength;
	_contentRead = other._contentRead;
	_host = other._host;
	_servConf = other._servConf;
}

Request&	Request::operator=(const Request& other) {
	if (this == &other)
		return *this;

	_firstLineChecked = other._firstLineChecked;
	_headerChecked = other._headerChecked;
	_readingFinished = other._readingFinished;
	_isChunked = other._isChunked;
	_type = other._type;
	delete _method;
	_method = NULL;
	if (other._method)
		_method = other._method->clone();
	delete _response;
	_response = NULL;
	if (other._response)
		_response = other._response->clone();
	_headerMap = other._headerMap;
	_contentLength = other._contentLength;
	_contentRead = other._contentRead;
	_host = other._host;
	_servConf = other._servConf;
	return *this;
}

bool		Request::operator==(const Request& other) const
{
	return (_firstLineChecked == other._firstLineChecked &&
			_headerChecked == other._headerChecked &&
			_readingFinished == other._readingFinished &&
			_isChunked == other._isChunked &&
			_type == other._type &&
			_method == other._method &&
			_headerMap == other._headerMap &&
			_contentLength == other._contentLength &&
			_contentRead == other._contentRead &&
			_host == other._host &&
			_servConf == other._servConf);
}

Request::~Request() {
	delete _method;
	delete _response;
}

bool	Request::hasMethod() const {
	if (_method)
		return true;
	return false;
}

std::string Request::getMethodName() const {
	if (this->_method)
		return this->_method->getName();
	throw	std::runtime_error("Server error 101");
}

std::string Request::getMethodPath() const {
	if (this->_method)
		return this->_method->getPath();
	throw	std::runtime_error("Server error 102");
}

std::string Request::getMethodProtocol() const {
	if (this->_method)
		return this->_method->getProtocol();
	return "HTTP/1.1";
}

std::string Request::getMethodMimeType() const {
	if (this->_method)
		return this->_method->getMimeType();
	return "text/plain"; // BP: or throw error?
}

bool	Request::getFirstLineChecked() const {
	return this->_firstLineChecked;
}

bool	Request::getReadingFinished() const {
	return this->_readingFinished;
}

std::map<std::string, std::string> Request::getHeaderMap() const {
	return this->_headerMap;
}

std::string Request::getHeaderFromHeaderMap(std::string headerName) const {
	if (_headerMap.find(headerName) == _headerMap.end())
		return "";
	return _headerMap.find(headerName)->second;
}

void Request::setMethodMimeType(std::string path) {
	this->_method->setMimeType(path);
}



// static void	checkLineLastChars(std::string& line) {
// 	if (!line.empty() && line[line.size() - 1] == '\n')
// 		line.resize(line.size() - 1);
// 	if (!line.empty() && line[line.size() - 1] == '\r')
// 		line.resize(line.size() - 1);
// }


static void	checkTelnetInterruption(std::vector<char>& line) {
	signed char stopTelnet[] = {-1, -12, -1, -3, 6};

	if (line.size() == 5 && std::equal(line.begin(), line.end(), stopTelnet)) {
		throw std::runtime_error(TELNETSTOP);
	}
}

void Request::storeOneHeaderInMap(const std::string& oneLine) {

	// std::cout << "$" << oneLine << "$" << std::endl;
	std::size_t pos = oneLine.find(":");
	if (pos == std::string::npos)
		return;
	std::string	key = oneLine.substr(0, pos);
	std::string value = oneLine.substr(pos + 1);
	while (!value.empty() && value[0] == ' ') {
		value.erase(0, 1);
	}
	if (_headerMap.find(key) == _headerMap.end()) {
		_headerMap[key] = value;
		// std::cout << key << "$" << value << std::endl;
	}
	else
		_headerMap[key] += value; // BP: to test, add delimiter
}

void Request::storeHeadersInMap(const std::string& strLine, std::size_t& endPos) {
	std::size_t pos = strLine.find("\r\n", endPos + 2);
	std::size_t pos2 = endPos;

	endPos = strLine.find("\r\n\r\n", 0);
	if (strLine.size() == 2 && strLine.find("\r\n") != std::string::npos) {
		_headerChecked = true;
		if (_method->getName() == "GET" || _method->getName() == "DELETE")
			_readingFinished = true;
		return;
	}
	if (endPos == std::string::npos) {
		// for request via telnet:
		storeOneHeaderInMap(strLine.substr(0));
		return;
	} else {
		_headerChecked = true;
		if (_method->getName() == "GET" || _method->getName() == "DELETE")
			_readingFinished = true;
	}
	while (pos <= endPos) {
		// std:: cout << "my pos: " << pos2 << " & " << pos << std::endl;
		storeOneHeaderInMap(strLine.substr(pos2 + 2, pos - (pos2 + 2)));
		pos2 = pos;
		pos = strLine.find("\r\n", pos2 + 2);
	}
}


void	Request::storeRequestBody(const std::string& strLine, std::size_t endPos) {
	std::size_t pos = strLine.find("filename=", endPos);
	char* end;
	unsigned long num = strtoul(getHeaderFromHeaderMap("Content-Length").c_str(), &end, 10);
	// std::atoi(getHeaderFromHeaderMap("Content-Length").c_str())
	if (pos == std::string::npos && num > strLine.substr(endPos).size() ) {
		return;
	}

	//-- SUMON commented this out to handle post without filename
	if (pos == std::string::npos)
	{
		//-- Find the start of the Content-Type header
		//-- It is useful to know if we have to decode URL
		std::string contentTypeHeader = "Content-Type: ";
		std::size_t contentTypePos = strLine.find(contentTypeHeader);
		if (contentTypePos != std::string::npos)
		{
			//-- Extract the Content-Type value
			std::size_t start = contentTypePos + contentTypeHeader.length();
			std::size_t end = strLine.find("\r\n", start);
			std::string contentType = strLine.substr(start, end - start);
		}

		//-- Find the start of the body (after the double CRLF)
		std::string bodyDelimiter = "\r\n\r\n";
		std::size_t bodyPos = strLine.find(bodyDelimiter);
		if (bodyPos != std::string::npos) {
			//-- Body starts after the double CRLF
			_requestBody = strLine.substr(bodyPos + bodyDelimiter.length());
		}
		_readingFinished = true;
		return;
	}

	_postFilename = strLine.substr(pos + 10, strLine.find('"', pos + 10) - pos - 10);
	// std::cout << _postFilename << "\n\n" << std::endl;
	pos = strLine.find("\r\n\r\n", endPos + 4);
	// std::cout << "rnrn: " << pos << std::endl;
	// std::cout << "$" << _postFilename << "$" << std::endl;
	std::map<std::string, std::string>::iterator it = _headerMap.find("Content-Type");
	// std::cout << "content: " << it->second << std::endl;
	std::string boundary;
	if (it != _headerMap.end()) {
		// BP: check when there is no boundary
		std::string contentType = it->second;
		size_t boundaryPos = contentType.find("boundary=");
		if (boundaryPos != std::string::npos) {
			boundary = "--" + contentType.substr(boundaryPos + 9);

			//-- SUMON :
			//-- first find the start and end positions of the file data
			//-- In the previous implementation, we used the boundary to find the start and end positions
			//-- So some sections after boundary were included in the file data
			//-- Which caused the file to be corrupted
			size_t boundaryPos = strLine.find(boundary, pos);
			size_t startPos = strLine.find("\r\n\r\n", boundaryPos);
			size_t endPos = strLine.find(boundary, startPos);

			_requestBody = strLine.substr(startPos + 4, endPos - startPos - 4); //-- 4 is for "\r\n\r\n" and --\r\n at the end
		}
	}
	_readingFinished = true;
	// Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryMLBLjWKqQwsOEKEd
	// std::string end =	strLine.rfind();
	// std::cout << "$" << _requestBody << "$" << std::endl;
}


void	Request::extractHttpMethod(std::string& requestLine)
{
	std::string	methodContainer = requestLine.substr(0, 8);
	size_t endPos = methodContainer.find_first_of(" ");
	if (endPos == std::string::npos)
		throw std::runtime_error("400"); // SUMON: sometimes this get thrown
	std::string method = requestLine.substr(0, endPos);
	requestLine.erase(0, endPos + 1);
	createHttpMethod(method);
}

void Request::createHttpMethod(const std::string& method) {
	// _bufferSize = 4096; //could be set at init already
	if (method == "GET")
		_method = new GetMethod();
	else if (method == "POST")
	{
		// _bufferSize = 8192;
		_method = new PostMethod();
	}
	// else if (method == "OPTIONS")
	// 	return new OptionsMethod();
	else if (method == "DELETE")
		_method = new DeleteMethod();
	else
		throw std::runtime_error("400");
	_method->setName(method);
}

void	Request::checkFirstLine(std::string& strLine, std::size_t& endPos) {
	// checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		return;
	}
	extractHttpMethod(strLine);
	std::size_t spacePos2 = strLine.find(" ");
	if (spacePos2 == std::string::npos)
		throw std::runtime_error("400");
	this->_method->setPath(strLine.substr(0, spacePos2));

	endPos = strLine.find("\r\n", spacePos2 + 1);
	if (endPos == std::string::npos)
		this->_method->setProtocol(strLine.substr(spacePos2 + 1));
	else {
		this->_method->setProtocol(strLine.substr(spacePos2 + 1, endPos - (spacePos2 + 1)));
	}
	_firstLineChecked = true;
}

void	Request::checkHost(Client* client) {

	std::map<std::string, std::string>::const_iterator it =_headerMap.find("Host");
	if (it == _headerMap.end())
		throw std::runtime_error("400 d");
	std::string host = it->second;
	std::size_t pos = host.find(':');
	host = host.substr(0, pos);
	_servConf = client->_socket->getConfig(host);
	if (!_servConf)
		_servConf = &(client->_socket->_configs.begin())->second;
	_host = host;
}


void	Request::executeMethod(int socketFd, Client *client)
{
	this->checkHost(client); // BP: first check which hostname else it would use the standardhostname
	this->_method->executeMethod(socketFd, client, *this);
}

//-- This is not ALLOWED
// int	Request::invalidRequest(Client* client)
// {
// 	if (errno == EINTR)
// 		return (-1);
// 	// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
// 	if (errno != EAGAIN && errno != EWOULDBLOCK)
// 		client->_server->_epoll->removeClient(client);	// Close the socket on other read errors
// 	return (-1); // Move to the next event
// }

int	Request::emptyRequest(Client* client)
{
	std::cout << "Client disconnected: FD " << client->getFd() << std::endl;
	client->_server->_epoll->removeClient(client);
	return (-1); // Move to the next event
}

void	Request::validRequest(Server* serv, std::vector<char> buffer, ssize_t count, Request& request)
{
	std::size_t endPos = 0;
	(void) serv;

	buffer.resize(count);
	checkTelnetInterruption(buffer);
	std::string strLine(buffer.begin(), buffer.end());

	std::vector<char> str = buffer;
	if(!request.getFirstLineChecked()) {
		checkFirstLine(strLine, endPos);
	}
	if (request.getFirstLineChecked() && !request._headerChecked && endPos != std::string::npos) {
		storeHeadersInMap(strLine, endPos);
	}
	//-- SUMON commented
	// if (request._firstLineChecked && request._headerChecked && !request._readingFinished && endPos != std::string::npos) {
	// 	if (this->_method->getName() == "POST") {
	// 		storeRequestBody(strLine, endPos);
	// 	}
	// }
	//-- OLD POST BODY
	// if (request._firstLineChecked && request._headerChecked && endPos != std::string::npos) {
	// 	if (this->_method->getName() == "POST") {
	// 		storeRequestBody(strLine, endPos);
	// 	}
	// }
}


//-- SUMON: Moved requestBody and totalBytesRead to global scope
//-- to handle multiple requests in the same connection
//-- Reset the values after each request
std::string requestBody;
ssize_t totalBytesRead = 0;

int Request::clientRequest(Client* client)
{
	int event_fd = client->getFd();
	bool writeFlag = false;
	bool contentLengthFound = false;

	std::vector<char> buffer(SOCKET_BUFFER_SIZE);

	try
	{
		buffer.resize(SOCKET_BUFFER_SIZE);
		ssize_t count = recv(event_fd, &buffer[0], buffer.size(), 0);
		if (count == -1)
		{
			//-- Maybe should write some error message
			//Helper::modifyEpollEventClient(*client->_server->_epoll, client, EPOLLIN | EPOLLET);
			return (1);
		}
		else if (count == 0)
			return emptyRequest(client);

		buffer.resize(count);
		validRequest(client->_server, buffer, count, client->_request);

		//-- Append data to requestBody
		requestBody.append(buffer.data(), count);
		totalBytesRead += count;

		//-- Check for Content-Length.
		//-- If has a content length, means request has a body.
		if (_headerChecked && !contentLengthFound)
		{
			std::map<std::string, std::string>::const_iterator it = _headerMap.find("Content-Length");
			if (it != _headerMap.end()) {
			 _contentLength = std::atoi(it->second.c_str());
			 contentLengthFound = true;
			}
		}

		//-- Stop reading if we have reached Content-Length
		if (contentLengthFound && totalBytesRead >= (ssize_t) _contentLength)
		{
			_readingFinished = true;
			totalBytesRead = 0;
		}

		//-- Process the request body if headers are fully checked and reading is finished
		if (_firstLineChecked && _headerChecked && _readingFinished)
		{
			//-- SUMON: client_max_body_size check moved to PostMethod
			if (this->_method->getName() == "POST")
			 storeRequestBody(requestBody, 0);
			requestBody.clear();
		}
		else
		{
			// Not finished reading, return to epoll event loop
			Helper::modifyEpollEventClient(*client->_server->_epoll, client, EPOLLIN | EPOLLET);
			return 0;
		}
	}
	catch (std::exception &e)
	{
		if (std::string(e.what()) == TELNETSTOP) {
			client->_server->_epoll->removeClient(client);
		} else {
			client->_request._response->error(client->_request, e.what(), client);
		}
		std::cerr << "Exception: " << e.what() << std::endl;
		writeFlag = true;
		return OK;
	}

	// Execute the method if reading is finished and no errors occurred
	if (!writeFlag && _readingFinished)
	{
		try {
			client->_request.executeMethod(event_fd, client);
		}
		catch (std::exception &e) {
			client->_request._response->error(client->_request, e.what(), client);
		}
	}

	writeFlag = false;
	std::cout << "-------------------------------------" << std::endl;
	return 0;
}


void	Request::requestReset() {
	_type = -1;
	_firstLineChecked = false;
	_headerChecked = false;
	_readingFinished = false;
	delete this->_method;
	_method = NULL;
	_headerMap.clear();
	_isChunked = false;
	_contentLength = 0;
	_contentRead = 0;
	_host = "";
	_requestBody = "";
	_postFilename = "";
	_servConf = NULL;
	delete _response;
	_response = new Response();
}

std::string	Request::getHost() const
{
	return (_host);
}

//-- BONUS : cookies
std::string Request::getSessionId() const
{
	// std::cout << _headerMap.size() << "\n\n" << std::endl;
	// for (std::map<std::string, std::string>::const_iterator it = _headerMap.begin(); it != _headerMap.end(); it++)
	// {
	// 	std::cout << "$" << it->first << ": " << it->second << "$" << std::endl;
	// }

	std::string cookie = getHeaderFromHeaderMap("Cookie");
	//std::cout << "\n\nCookie: " << cookie << std::endl;
	std::size_t pos = cookie.find("session=");
	if (pos == std::string::npos)
		return ("");
	std::size_t end = cookie.find(";", pos);
	std::string _sessionId = cookie.substr(pos + 10, end - pos - 10);
	return (_sessionId);
}





// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
// Accept-Encoding: gzip, deflate, br, zstd
// Accept-Language: en-US,en;q=0.9
// Connection: keep-alive
// Cookie: session=lVH6A0QDpBNrQNfXfPPCeQSj1gIuFJWQ
// Host: 127.0.0.1:8000
// Referer: http://127.0.0.1:8000/about/index.html
// Sec-Fetch-Dest: document
// Sec-Fetch-Mode: navigate
// Sec-Fetch-Site: same-origin
// Sec-Fetch-User: ?1
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/130.0.0.0 Safari/537.36
// sec-ch-ua: "Chromium";v="130", "Google Chrome";v="130", "Not?A_Brand";v="99"
// sec-ch-ua-mobile: ?0
// sec-ch-ua-platform: "Linux"