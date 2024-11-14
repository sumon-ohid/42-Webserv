#include <stdexcept>
#include <string>

#include "../includes/Method.hpp"
#include "../includes/Helper.hpp"



const std::string Method::_methodArray[ARRAY_SIZE] = {"GET", "POST", "DELETE"};

Method::Method() {
	_name = "";
	_path = "";
	_protocol = "HTTP/1.1";
	_mimeType = "";
}

Method::Method(const Method& other) : _name(other._name), _path(other._path), _protocol(other._protocol), _mimeType(other._mimeType) {}

Method& Method::operator=(const Method& other) {
	if (this == &other)
		return *this;
	_name = other._name;
	_path = other._path;
	_protocol = other._protocol;
	_mimeType = other._mimeType;
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
	return _name;
}

std::string Method::getPath() const {
	return _path;
}

std::string Method::getProtocol() const {
	return _protocol;
}

std::string Method::getMimeType() const {
	return _mimeType;
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

	//-- If path is encoded, this will decode it.
	path = Helper::decodeUrl(path);
	if (path.find("..") != std::string::npos)
		throw std::runtime_error("403");

	this->_path = path;
}

void	Method::setProtocol(std::string protocol) {
	if (protocol.empty()) {
		throw std::runtime_error("505");
	}
	if (protocol == "HTTP/2" || protocol == "HTTP/1.0")
		throw std::runtime_error("505");
	if (protocol != "HTTP/1.1")
		throw std::runtime_error("400");
	this->_protocol = protocol;
}

void	Method::setMimeType(std::string& path) {
	size_t endPos = path.rfind('.');
	if (endPos == std::string::npos)
		return;
	std::string ending = path.substr(endPos);
	for (std::map<std::string, std::string>::const_iterator it = Helper::mimeTypes.begin(); it != Helper::mimeTypes.end(); it++) {
		if (ending == it->first)
			this->_mimeType = it->second;
	}
	if (_mimeType.empty()) {
		throw std::runtime_error("415");
	}
}
