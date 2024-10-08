#include <cstddef>
#include <stdexcept>
#include <string>
#include <algorithm>

#include "Request.hpp"
#include "GetMethod.hpp"
#include "main.hpp"

Request::Request() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	this->_method = NULL;
}

Request::Request(const Request& other) {
	(void) other;
}

Request& Request::operator=(const Request& other) {
	if (this == &other)
		return *this;
	// copy variables here
	return *this;
}

Request::~Request() {
	delete this->_method;
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

bool	Request::getFirstLineChecked() const {
	return this->_firstLineChecked;
}

bool	Request::getReadingFinished() const {
	return this->_readingFinished;
}

std::map<std::string, std::string> Request::getHeaderMap() const {
	return this->_headerMap;
}

#include <iostream>

static void	checkLineLastChars(std::string& line) {
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.resize(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}


static void	checkInterruption(std::vector<char>& line) {
	char stopTelnet[] = {-1, -12, -1, -3, 6};

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
	std::cout << "$" << key << "$" << value << "$" << std::endl;
}

void	Request::checkFirstLine(std::vector<char>& line) {
	std::cout << "firstlineNotchecked" << std::endl;
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
	std::cout << "done" << std::endl;
}

void	Request::checkLine(std::vector<char>& line) {
	std::cout << "checkline_start" << std::endl;
	checkInterruption(line);
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		this->_readingFinished = true;
		return;
	}
	checkOneLine(strLine);
	std::cout << "checkline_end" << std::endl;
	// if (value.empty())
	// 	throw std::runtime_error("empty value string");
}

void	Request::executeMethod(int socketFd)  {
	this->_method->executeMethod(socketFd, *this);
}


void	Request::requestReset() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	this->_method = NULL;
}
