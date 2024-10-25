#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>
#include <errno.h>
#include <sys/types.h>

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
	_response = other._response->clone();
	_headerMap = other._headerMap;
	_contentLength = other._contentLength;
	_contentRead = other._contentRead;
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
	_response = other._response->clone();
	_headerMap = other._headerMap;
	_contentLength = other._contentLength;
	_contentRead = other._contentRead;
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
			_contentRead == other._contentRead);
}

Request::~Request() {
	delete this->_method;
	delete this->_response;
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



static void	checkLineLastChars(std::string& line) {
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.resize(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}


static void	checkTelnetInterruption(std::vector<char>& line) {
	signed char stopTelnet[] = {-1, -12, -1, -3, 6};

	if (line.size() == 5 && std::equal(line.begin(), line.end(), stopTelnet)) {
		throw std::runtime_error(TELNETSTOP);
	}
}

void Request::storeHeadersInMap(const std::string& oneLine) {
	std::size_t pos = oneLine.find(":");
	if (pos == std::string::npos)
		return;
	std::string	key = oneLine.substr(0, pos);
	std::string value = oneLine.substr(pos + 1);
	while (!value.empty() && value[0] == ' ') { //BP: maybe also on the end?
		value.erase(0, 1);
	}
	if (_headerMap.find(key) != _headerMap.end())
		_headerMap[key] = value;
	else
		_headerMap[key] += value; // BP: to test
}

void	Request::storeRequestBody(const std::string& strLine, std::size_t pos, std::size_t endPos) {
	pos = strLine.find("filename=");

	_postFilename = strLine.substr(pos + 10, strLine.find('"', pos + 10) - pos - 10);
	pos = strLine.find("\r\n\r\n", endPos + 4);
	// std::cout << "$" << _postFilename << "$" << std::endl;
	std::map<std::string, std::string>::iterator it = _headerMap.find("Content-Type");
	// std::cout << it->second << std::endl;
	std::string boundary;
	if (it != _headerMap.end()) {
		boundary = "--" + it->second.substr(it->second.find("boundary=") + 9);
	}
	// Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryMLBLjWKqQwsOEKEd
	// std::string end =  strLine.rfind();
	endPos = strLine.find(boundary, pos + 4);
	_requestBody = strLine.substr(pos + 4, endPos - pos - 7);
	// std::cout << "$" << _requestBody << "$" << std::endl;
}

void	Request::checkSentAtOnce(const std::string& strLine, std::size_t pos1, std::size_t pos2) {
		this->_method->setProtocol(strLine.substr(pos1 + 1, pos2 - (pos1 + 1)));
		std::size_t pos = strLine.find("\r\n", pos2 + 1);
		std::size_t endPos = strLine.find("\r\n\r\n", pos2 + 1);
		while (pos < endPos) {
			storeHeadersInMap(strLine.substr(pos2 + 2, pos - (pos2 + 2)));
			pos2 = pos;
			pos = strLine.find("\r\n", pos2 + 1);
		}

		if (this->_method->getName() == "POST") {
			storeRequestBody(strLine, pos, endPos);
		}
		// _readingFinished = true;
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

void	Request::checkFirstLine(std::vector<char>& line) {
	// this->_method = new GetMethod(); // BP: only to not segfault when we have to escape earlier
	checkTelnetInterruption(line);
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		return;
	}
	extractHttpMethod(strLine);
	// std::size_t spacePos = strLine.find(" ", 0);
	// std::string	methodName = strLine.substr(0, spacePos);
	// if (spacePos == std::string::npos)
	// 	throw std::runtime_error("400");
	// delete this->_method;
	// this->_method = new GetMethod();
	// // check which method

	// this->_method->setName(methodName);

	std::cout << _method->getName() << std::endl;

	std::size_t spacePos2 = strLine.find(" ");
	if (spacePos2 == std::string::npos)
		throw std::runtime_error("400");
	this->_method->setPath(strLine.substr(0, spacePos2));

	std::size_t spacePos3 = strLine.find("\r\n", spacePos2 + 1);
	if (spacePos3 == std::string::npos)
		this->_method->setProtocol(strLine.substr(spacePos2 + 1));
	else {
		checkSentAtOnce(strLine, spacePos2, spacePos3);
	}
	_firstLineChecked = true;
}

void	Request::checkLine(std::vector<char>& line) {
	checkTelnetInterruption(line);
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		this->_readingFinished = true;
		return;
	}
	storeHeadersInMap(strLine);
}

void	Request::checkHost(ServerConfig& config) const {
	std::map<std::string, std::string>::const_iterator it =_headerMap.find("Host");
	if (it == _headerMap.end())
		throw std::runtime_error("400");
	std::string host = it->second;
	std::size_t pos = host.find(':');
	host = host.substr(0, pos);
	std::cout << "host: $" << host << "$, fromServer: $" << config.getServerName() << "$" << std::endl;
	if (host != config.getServerName())
		throw std::runtime_error("404"); // BP: to check if correct value
}

//-- SUMON: I am working on this function
void	Request::executeMethod(int socketFd, Client *client)
{
	// this->checkHost(config); // BP: first check which hostname else it would use the standardhostname
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
	(void) serv;
	// std::cout << "test1: " << request.getFirstLineChecked() << std::endl;
	buffer.resize(count);
	// if (_buffer.size() == 5)
	// std::cout << (int) (unsigned char)_buffer[0] << " & " << (int) (unsigned char)_buffer[1] << " & " << (int) (unsigned char)_buffer[2] << " & " << (int) (unsigned char)_buffer[3] << " & " << (int) (unsigned char)_buffer[4] << " & " << _buffer.size() << std::endl;
	if(!request.getFirstLineChecked()) {
		checkFirstLine(buffer);
	}
	if (request.getFirstLineChecked() && !request._headerChecked) {
		// storeHeadersInMap(strLine.substr(pos2 + 2, pos - (pos2 + 2)));
	}
	if (request._firstLineChecked && request._headerChecked) {
		request.checkLine(buffer);
		// if (this->_method->getName() == "POST") {
		// 	storeRequestBody(strLine, pos, endPos);
		// }
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
				Response::error(event_fd, client->_request, static_cast<std::string>(e.what()), client);
			}
			std::cout << "exception: " << e.what() << std::endl;
			writeFlag = true;
			return OK;
		}
	// }
	if (!writeFlag)
	{
		try {
			client->_request.executeMethod(event_fd, client);
		}
		catch (std::exception &e) {
			Response::error(event_fd, client->_request, static_cast<std::string>(e.what()), client);
		}

	}
	(void) count;
	writeFlag = false;
	// std::map<std::string, std::string> testMap = client->_request.getHeaderMap();
	std::cout << client->_request.getMethodName() << " " << client->_request.getMethodPath() << " " << client->_request.getMethodProtocol() << std::endl;
	std::cout << "-------------------------------------" << std::endl;
	client->_request.requestReset();
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

Method*	Request::GetMethodClass() const
{
	if (_method)
		return (_method);
	return (NULL);
}
