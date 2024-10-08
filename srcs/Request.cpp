#include <cstddef>
#include <stdexcept>
#include <string>

#include "Request.hpp"
#include "GetHeadMethod.hpp"

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

std::string Request::getMethodName() {
	if (this->_method)
		return this->_method->getName();
	throw	std::runtime_error("Server error 101");
}

std::string Request::getMethodPath() {
	if (this->_method)
		return this->_method->getPath();
	throw	std::runtime_error("Server error 102");
}

std::string Request::getMethodProtocol() {
	if (this->_method)
		return this->_method->getProtocol();
	return "HTTP/1.1"; // to also return when method is wrong
}

bool	Request::getFirstLineChecked() {
	return this->_firstLineChecked;
}

bool	Request::getReadingFinished() {
	return this->_readingFinished;
}

#include <iostream>

static void	checkLineLastChars(std::string& line) {
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.resize(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}

void	Request::checkFirstLine(std::vector<char>& line) {
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		return;
	}
	std::size_t spacePos = strLine.find(" ", 0);
	std::string	methodName = strLine.substr(0, spacePos);
	if (spacePos == std::string::npos)
		throw std::runtime_error("400 Bad Request");
	this->_method = new GetHeadMethod();
	// check which method

	this->_method->setName(methodName);
	std::cout << "$" << _method->getName() << "$" << std::endl;

	std::size_t spacePos2 = strLine.find(" ", spacePos + 1);
	if (spacePos2 == std::string::npos)
		throw std::runtime_error("400 Bad Request");
	this->_method->setPath(strLine.substr(spacePos + 1, spacePos2 - (spacePos + 1)));
	std::cout << "$" << _method->getPath() << "$" << std::endl;

	std::size_t spacePos3 = strLine.find("\r\n", spacePos2 + 1);
	if (spacePos3 == std::string::npos)
		this->_method->setProtocol(strLine.substr(spacePos2 + 1));
	else
		this->_method->setProtocol(strLine.substr(spacePos2 + 1, spacePos3 - (spacePos2 + 1)));
	std::cout << "$" << _method->getProtocol() << "$" << std::endl;
	_firstLineChecked = true;
}

void	Request::checkLine(std::vector<char>& line) {
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		this->_readingFinished = true;
		return;
	}

	std::size_t pos = strLine.find(":");
	if (pos == std::string::npos)
		return;
	std::string	key = strLine.substr(0, pos);
	std::string value = strLine.substr(pos + 1);
	while (!value.empty() && value[0] == ' ') {
		value.erase(0, 1);
	}
	// if (value.empty())
	// 	throw std::runtime_error("empty value string");

	std::cout << "$" << key << "$" << value << "$" << std::endl;
}


void	Request::requestReset() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	this->_method = NULL;
}
