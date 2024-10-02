#include <cstddef>
#include <stdexcept>
#include <string>

#include "Header.hpp"

Header::Header() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
}

// Header::Header(std::string method) {

// }

Header::Header(const Header& other) {
	(void) other;
}

Header& Header::operator=(const Header& other) {
	if (this == &other)
		return *this;
	// copy variables here
	return *this;
}

Header::~Header() {}

std::string Header::getMethodName() {
	return this->_method.getName();
}

std::string Header::getMethodPath() {
	return this->_method.getPath();
}

std::string Header::getMethodProtocol() {
	return this->_method.getProtocol();
}

bool	Header::getFirstLineChecked() {
	return this->_firstLineChecked;
}

bool	Header::getReadingFinished() {
	return this->_readingFinished;
}

#include <iostream>

static void	checkLineLastChars(std::string& line) {
	if (!line.empty() && line[line.size() - 1] == '\n')
		line.resize(line.size() - 1);
	if (!line.empty() && line[line.size() - 1] == '\r')
		line.resize(line.size() - 1);
}

void	Header::checkFirstLine(std::vector<char>& line) {
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		return;
	}
	std::size_t spacePos = strLine.find(" ", 0);
	if (spacePos == std::string::npos)
		throw std::runtime_error("space in header not found");
	this->_method.setName(strLine.substr(0, spacePos));
	std::cout << "$" << _method.getName() << "$" << std::endl;

	std::size_t spacePos2 = strLine.find(" ", spacePos + 1);
	this->_method.setPath(strLine.substr(spacePos + 1, spacePos2 - (spacePos + 1)));
	std::cout << "$" << _method.getPath() << "$" << std::endl;

	this->_method.setProtocol(strLine.substr(spacePos2 + 1));
	std::cout << "$" << _method.getProtocol() << "$" << std::endl;
	_firstLineChecked = true;
}

void	Header::checkLine(std::vector<char>& line) {
	std::string strLine(line.begin(), line.end());
	checkLineLastChars(strLine);
	if (strLine.length() == 0) {
		this->_readingFinished = true;
		return;
	}

	std::size_t pos = strLine.find(":");
	if (pos == std::string::npos)
		throw std::runtime_error("no ':' in line");
	std::string	key = strLine.substr(0, pos);
	std::string value = strLine.substr(pos + 1);
	while (!value.empty() && value[0] == ' ') {
		value.erase(0, 1);
	}
	// if (value.empty())
	// 	throw std::runtime_error("empty value string");

	std::cout << "$" << key << "$" << value << "$" << std::endl;
}


void	Header::headerReset() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
	this->_method = Method();
}