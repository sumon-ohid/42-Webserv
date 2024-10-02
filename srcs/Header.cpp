#include "Header.hpp"

Header::Header() {
	this->_type = -1;
	this->_firstLineChecked = false;
	this->_readingFinished = false;
}

Header::Header(std::string method) {

}

Header::Header(const Header& other) {}

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

// void	Header::setFirstLineChecked() {
// 	this->_firstLineChecked = true;
// }

void	Header::checkFirstLine(std::vector<char> line) {
	if (line.size() == 2)
		return;
	_firstLineChecked = true;
}