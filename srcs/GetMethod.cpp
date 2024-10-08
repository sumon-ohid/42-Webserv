#include "GetMethod.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Request.hpp"
// #include <iostream>
#include <fstream>
#include <sstream>

GetMethod::GetMethod() : Method() {}

GetMethod::GetMethod(const GetMethod& other) : Method(other) {}

GetMethod&	GetMethod::operator=(const GetMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
	return *this;
}

GetMethod::~GetMethod() {}

void	GetMethod::executeMethod(int socketFd, Request& request) const {

	std::string body;
	std::string path = "./conf/webpage/"; //BP: should be from config file

	if (request.getMethodPath() == "/")
		path += "index.html";
	else
		path += request.getMethodPath();

	std::ifstream file(path.c_str()); // BP: check for file extension to send right mime type
	if (!file.is_open())
		throw std::runtime_error("404");
	std::ostringstream buffer;
	buffer << file.rdbuf();
	body = buffer.str();
	std::cout << "test" << std::endl;
	std::cout << body << std::endl;

	file.close();
	// if (this->_name == GET)
	Response::headerAndBody(socketFd, request, body);
	// else if (this->_name == HEAD)
	// 	Response::header(socketFd, request, body);

}
