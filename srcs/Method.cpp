#include <stdexcept>
#include <string>

#include "Method.hpp"

const std::string Method::_mArray[ARRAY_SIZE] = {"OPTIONS", "HEAD", "GET", "POST", "DELETE"};

Method::Method() {
	this->_name = "";
	this->_path = "";
	this->_protocol = "HTTP/1.1";
}

Method::Method(const Method& other) : _name(other._name), _path(other._path), _protocol(other._protocol) {}

Method& Method::operator=(const Method& other) {
	if (this == &other)
		return *this;
	this->_name = other._name;
	this->_path = other._path;
	this->_protocol = other._protocol;
	return *this;
}

bool	Method::operator==(const Method& other) const
{
	return (_name == other._name &&
			_path == other._path &&
			_protocol == other._protocol);
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
		throw std::runtime_error("400");

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (_mArray[i] == name) {
			this->_name = name;
			return;
		}
	}
	throw std::runtime_error("400");
}

void	Method::setPath(std::string path) {
	if (path.empty())
		throw std::runtime_error("400");
	this->_path = path;
}

#include <iostream>

void	Method::setProtocol(std::string protocol) {
	if (protocol.empty()) {
		this->_protocol = "HTTP/0.9";
		return;
	}
	std::cout << "$" << protocol << "$" << std::endl;
	if (protocol == "HTTP/2")
		throw std::runtime_error("505");
	if (protocol != "HTTP/1.1")
		throw std::runtime_error("400"); // BP check for other possibilities like 1.0001 etc.
	this->_protocol = protocol;
}
