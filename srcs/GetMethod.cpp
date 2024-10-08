#include "GetMethod.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Request.hpp"

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


	if (this->_name == GET)
		Response::headerAndBody(socketFd, request, body);
	else if (this->_name == HEAD)
		Response::header(socketFd, request, body);

}
