#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>
#include <errno.h>

#include "../includes/Request.hpp"
#include "../includes/Client.hpp"
#include "../includes/GetMethod.hpp"
#include "../includes/main.hpp"
#include "../includes/Response.hpp"
#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"

Request::Request() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	this->_method = NULL;
	this->_response = new Response();
}

Request::Request(const Request& other) {
	_firstLineChecked = other._firstLineChecked;
	_readingFinished = other._readingFinished;
	_type = other._type;
	if (other._method)
		_method = other._method->clone();
	else
	 	_method = NULL;
	this->_response = other._response->clone();
	_headerMap = other._headerMap;
}

Request&	Request::operator=(const Request& other) {
	if (this == &other)
		return *this;

	_firstLineChecked = other._firstLineChecked;
	_readingFinished = other._readingFinished;
	_type = other._type;
	delete _method;
	_method = NULL;
	if (other._method)
		_method = other._method->clone();
	delete _response;
	_response = other._response->clone();
	_headerMap = other._headerMap;
	return *this;
}

bool		Request::operator==(const Request& other) const
{
	return (_firstLineChecked == other._firstLineChecked &&
			_readingFinished == other._readingFinished &&
			_type == other._type &&
			_method == other._method &&
			_headerMap == other._headerMap);
}

Request::~Request() {
	delete this->_method;
	delete this->_response;
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
	return "HTTP/1.1"; // to also return when method is wrong
}

std::string Request::getMethodMimeType() const {
	if (this->_method)
		return this->_method->getMimeType();
	return "text/plain"; // BP: or throw error
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

#include <iostream>

static void	checkLineLastChars(std::string& line) {
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.resize(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}


static void	checkInterruption(std::vector<char>& line) {
	signed char stopTelnet[] = {-1, -12, -1, -3, 6};

	if (line.size() == 5 && std::equal(line.begin(), line.end(), stopTelnet)) {
		throw std::runtime_error(TELNETSTOP);
	}
}

void Request::checkOneLine(std::string oneLine) {
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
		_headerMap[key] += value;
	// std::cout << "$" << key << "$" << value << "$" << std::endl;
}

void	Request::checkFirstLine(std::vector<char>& line) {
	this->_method = new GetMethod(); // BP: only to not segfault when we have to escape earlier
	checkInterruption(line);
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		return;
	}
	std::size_t spacePos = strLine.find(" ", 0);
	std::string	methodName = strLine.substr(0, spacePos);
	if (spacePos == std::string::npos)
		throw std::runtime_error("400");
	delete this->_method;
	this->_method = new GetMethod();
	// check which method

	this->_method->setName(methodName);

	std::size_t spacePos2 = strLine.find(" ", spacePos + 1);
	if (spacePos2 == std::string::npos)
		throw std::runtime_error("400");
	this->_method->setPath(strLine.substr(spacePos + 1, spacePos2 - (spacePos + 1)));

	std::size_t spacePos3 = strLine.find("\r\n", spacePos2 + 1);
	if (spacePos3 == std::string::npos)
		this->_method->setProtocol(strLine.substr(spacePos2 + 1));
	else {
		// multiline input
		this->_method->setProtocol(strLine.substr(spacePos2 + 1, spacePos3 - (spacePos2 + 1)));
		std::size_t pos = strLine.find("\r\n", spacePos3 + 1);
		while (pos != std::string::npos) {
			checkOneLine(strLine.substr(spacePos3 + 2, pos - (spacePos3 + 2)));
			spacePos3 = pos;
			pos = strLine.find("\r\n", spacePos3 + 1);
		}
		_readingFinished = true;
	}
	_firstLineChecked = true;
}

void	Request::checkLine(std::vector<char>& line) {
	// std::cout << "checkline_start" << std::endl;
	checkInterruption(line);
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		this->_readingFinished = true;
		return;
	}
	checkOneLine(strLine);
	// std::cout << "checkline_end" << std::endl;
	// if (value.empty())
	// 	throw std::runtime_error("empty value string");
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
	// this->checkHost(config); // BP: activate when reading of servername is corrected
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
	if(request.getFirstLineChecked()) {
		request.checkLine(buffer);
	} else {
		request.checkFirstLine(buffer);
	}
}

int	Request::clientRequest(Client* client)
{

	// Read data from the client
	int		event_fd = client->getFd();
	bool	writeFlag = false;
	std::vector<char> buffer;  // Zero-initialize the buffer for incoming data
	ssize_t count = read(event_fd, &buffer[0], buffer.size());  // Read data from the client socket

	while (!client->_request.getReadingFinished())
	{
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
	}
	if (!writeFlag)
	{
		try {
			client->_request.executeMethod(event_fd, client);
		}
		catch (std::exception &e) {
			Response::error(event_fd, client->_request, static_cast<std::string>(e.what()), client);
		}

		std::cout << "-------------------------------------" << std::endl;
	}
	(void) count;
	writeFlag = false;
	std::map<std::string, std::string> testMap = client->_request.getHeaderMap();
	std::cout << client->_request.getMethodName() << " " << client->_request.getMethodPath() << " " << client->_request.getMethodProtocol() << std::endl;
	std::cout << "map size: " << testMap.size() << std::endl;
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
