#include <stdexcept>
#include <string>

#include "Method.hpp"

const std::string Method::_mArray[ARRAY_SIZE] = {"OPTIONS", "HEAD", "GET", "POST", "DELETE"};

Method::Method() {
	this->_name = "";
	this->_path = "";
	this->_protocol = "";
}

// Method::Method(const std::string name, const std::string path, const std::string protocol) {

// 	for (int i = 0; i < ARRAY_SIZE; i++) {
// 		if (_mArray[i] == name) {
// 			this->_name = name;
// 		}
// 	}
// 	if (this->_name.empty())
// 		throw std::runtime_error("not a valid method");

// }

Method::Method(const Method& other) : _name(other._name) {}

Method& Method::operator=(const Method& other) {
	if (this == &other)
		return *this;
	this->_name = other._name;
	return *this;
}

Method::~Method() {}

std::string Method::getName() const {
	return this->_name;
}

std::string Method::getPath() const {
	return this->_path;
}

std::string Method::getProtocol() const {
	return this->_protocol;
}

void	Method::setName(std::string name) {
	if (name.empty())
		throw std::runtime_error("not a valid method");
	// check spaces before

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (_mArray[i] == name) {
			this->_name = name;
			return;
		}
	}
	throw std::runtime_error("not a valid method");
}

void	Method::setPath(std::string path) {
	if (path.empty())
		throw std::runtime_error("not a valid method");
	this->_path = path;
}

void	Method::setProtocol(std::string protocol) {
	// if (protocol != "HTTP/1.1")
	// 	throw std::runtime_error("not a valid method");
	this->_protocol = protocol;
}
