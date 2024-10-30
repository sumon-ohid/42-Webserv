#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>
#include <errno.h>
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
	std::size_t pos = strLine.find("\r\n", endPos);
	std::size_t pos2 = endPos;

	endPos = strLine.find("\r\n\r\n", 0);
	if (strLine.size() == 2 && strLine.find("\r\n") != std::string::npos) {
		_headerChecked = true;
		if (_method->getName() == "GET")
			_readingFinished = true;
		return;
	}

	if (endPos == std::string::npos) {
		// for request via telnet:
		storeOneHeaderInMap(strLine.substr(0));
		return;
	} else {
		_headerChecked = true;
		if (_method->getName() == "GET")
			_readingFinished = true;
	}
	while (pos < endPos) {
		storeOneHeaderInMap(strLine.substr(pos2 + 2, pos - (pos2 + 2)));
		pos2 = pos;
		pos = strLine.find("\r\n", pos2 + 1);
	}
}


void	Request::storeRequestBody(const std::string& strLine, std::size_t endPos) {
	std::size_t pos = strLine.find("filename=");

	_postFilename = strLine.substr(pos + 10, strLine.find('"', pos + 10) - pos - 10);
	pos = strLine.find("\r\n\r\n", endPos + 4);
	// std::cout << "rnrn: " << pos << std::endl;
	// std::cout << "$" << _postFilename << "$" << std::endl;
	std::map<std::string, std::string>::iterator it = _headerMap.find("Content-Type");
	// std::cout << "content: " << it->second << std::endl;
	std::string boundary;
	if (it != _headerMap.end()) {
		boundary = "--" + it->second.substr(it->second.find("boundary=") + 9);
		endPos = strLine.find(boundary, pos + 4);
		_requestBody = strLine.substr(pos + 4, endPos - pos - 7);
	}
	_readingFinished = true;
	// Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryMLBLjWKqQwsOEKEd
	// std::string end =  strLine.rfind();
	// std::cout << "$" << _requestBody << "$" << std::endl;
}


void	Request::extractHttpMethod(std::string& requestLine)
{
	std::string	methodContainer = requestLine.substr(0, 8);
	size_t endPos = methodContainer.find_first_of(" ");
	if (endPos == std::string::npos)
		throw std::runtime_error("400");
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

// void	Request::checkLine(std::vector<char>& line) {
// 	checkTelnetInterruption(line);
// 	std::string strLine(line.begin(), line.end());
// 	checkLineLastChars(strLine);
// 	if (strLine.length() == 0) {
// 		this->_readingFinished = true;
// 		return;
// 	}
// 	storeHeadersInMap(strLine);
// }

void	Request::checkHost(Client* client) {

	std::cout << "printing header map:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it =_headerMap.begin(); it != _headerMap.end(); ++it)
		std::cout << it->first << ":" << it->second << std::endl;

	std::map<std::string, std::string>::const_iterator it =_headerMap.find("Host");
	if (it == _headerMap.end())
		throw std::runtime_error("400");
	std::string host = it->second;
	std::size_t pos = host.find(':');
	host = host.substr(0, pos);
	std::cout << "print host:\t" << host << std::endl;
	_servConf = client->_socket->getConfig(host);

	std::cout << "printing configs:" << std::endl;
	for (mHstConfs::iterator it2 = client->_socket->_configs.begin(); it2 != client->_socket->_configs.end(); ++it2)
		std::cout << "hostname stored to retrieve config:\t" << it2->first << std::endl;


	std::cout << "\n\noutput:" << (client->_socket->_configs.begin())->first << std::endl;

	if (!_servConf)
		_servConf = &(client->_socket->_configs.begin())->second;


	_host = host;
}

//-- SUMON: I am working on this function
void	Request::executeMethod(int socketFd, Client *client)
{
	this->checkHost(client); // BP: first check which hostname else it would use the standardhostname
	this->_method->executeMethod(socketFd, client, *this);
}

int	Request::invalidRequest(Client* client)
{
	if (errno == EINTR)
		return (-1);
	// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		client->_server->_epoll->removeClient(client);  // Close the socket on other read errors
	return (-1); // Move to the next event
}

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
	if(!request.getFirstLineChecked()) {
		checkFirstLine(strLine, endPos);
	}
	if (request.getFirstLineChecked() && !request._headerChecked && endPos != std::string::npos) {
		storeHeadersInMap(strLine, endPos);
	}
	if (request._firstLineChecked && request._headerChecked && !request._readingFinished && endPos != std::string::npos) {
		if (this->_method->getName() == "POST") {
			storeRequestBody(strLine, endPos);
		}
	}
}

int	Request::clientRequest(Client* client)
{
	int		event_fd = client->getFd();
	bool	writeFlag = false;
	std::vector<char> buffer;  // Zero-initialize the buffer for incoming data
	ssize_t count = read(event_fd, &buffer[0], buffer.size());  // Read data from the client socket

	// while (!client->_request.getReadingFinished())
	// {
		try
		{
			buffer.resize(SOCKET_BUFFER_SIZE);
			ssize_t count = read(event_fd, &buffer[0], buffer.size());
			if (count == -1)
				return (invalidRequest(client));
			else if (count == 0)
				return (emptyRequest(client));
			else
				validRequest(client->_server, buffer, count, client->_request);
		}
		catch (std::exception &e)
		{
			if (static_cast<std::string>(e.what()) == TELNETSTOP) {
				client->_server->_epoll->removeClient(client);
			} else {
				client->_request._response->error(event_fd, client->_request, static_cast<std::string>(e.what()), client);
			}
			std::cerr << "exception: " << e.what() << std::endl;
			writeFlag = true;
			return OK;
		}
	// }
	// std::cout << _readingFinished << std::endl;
	if (!writeFlag && _readingFinished)
	{
		try {
			client->_request.executeMethod(event_fd, client);
			// std::cout << client->_request.getHeaderMap().size() << std::endl;
			client->_request.requestReset();
		}
		catch (std::exception &e) {
			client->_request._response->error(event_fd, client->_request, static_cast<std::string>(e.what()), client);
			client->_request.requestReset();
		}

	}
	(void) count;
	writeFlag = false;
	// std::map<std::string, std::string> testMap = client->_request.getHeaderMap();
	// std::cout << client->_request.getMethodName() << " " << client->_request.getMethodPath() << " " << client->_request.getMethodProtocol() << std::endl;
	std::cout << "-------------------------------------" << std::endl;
	return (0);
}


void	Request::requestReset() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	delete this->_method;
	this->_method = NULL;
	this->_headerMap.clear();
}

std::string	Request::getHost() const
{
	return (_host);
}
