#include "GetHeadMethod.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Request.hpp"

GetHeadMethod::GetHeadMethod() : Method() {}

GetHeadMethod::GetHeadMethod(const GetHeadMethod& other) : Method(other) {}

GetHeadMethod&	GetHeadMethod::operator=(const GetHeadMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
	return *this;
}

GetHeadMethod::~GetHeadMethod() {}

void	GetHeadMethod::executeMethod(int socketFd, Request& request) const {

	std::string body;


	if (this->_name == GET)
		Response::headerAndBody(socketFd, request, body);
	else if (this->_name == HEAD)
		Response::header(socketFd, request, body);

}
