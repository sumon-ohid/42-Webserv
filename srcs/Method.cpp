#include <stdexcept>
#include <string>

#include "Method.hpp"

static std::map<std::string, std::string> initMimeMap() {
	std::map<std::string, std::string> mimes;
	mimes[".css"] = "text/css";
	mimes[".html"] = "text/html";
	mimes[".js"] = "text/javascript";
	return mimes;
}

const std::map<std::string, std::string> Method::mimeTypes = initMimeMap();

const std::string Method::_methodArray[ARRAY_SIZE] = {"OPTIONS", "HEAD", "GET", "POST", "DELETE"};

Method::Method() {
	this->_name = "";
	this->_path = "";
	this->_protocol = "HTTP/1.1";
	this->_mimeType = "";
}

Method::Method(const Method& other) : _name(other._name), _path(other._path), _protocol(other._protocol) {}

Method& Method::operator=(const Method& other) {
	if (this == &other)
		return *this;
	this->_name = other._name;
	this->_path = other._path;
	this->_protocol = other._protocol;
	this->_mimeType = other._mimeType;
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

std::string Method::getMimeType() const {
	return this->_mimeType;
}

void	Method::setName(std::string name) {
	if (name.empty())
		throw std::runtime_error("400");

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (_methodArray[i] == name) {
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
		this->_protocol = "HTTP/0.9"; // BP: to check what is different
		return;
	}
	if (protocol == "HTTP/2")
		throw std::runtime_error("505");
	if (protocol != "HTTP/1.1")
		throw std::runtime_error("400"); // BP check for other possibilities like 1.0001 etc.
	this->_protocol = protocol;
}

void	Method::setMimeType(std::string& path) {
	size_t endPos = path.rfind('.');
	if (endPos == std::string::npos)
		return;
	std::string ending = path.substr(endPos);
	for (std::map<std::string, std::string>::const_iterator it = mimeTypes.begin(); it != mimeTypes.end(); it++) {
		if (ending == it->first)
			this->_mimeType = it->second;
	}
}
