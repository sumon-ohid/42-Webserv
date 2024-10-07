#include "GetHeadMethod.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Header.hpp"

GetHeadMethod::GetHeadMethod() : Method() {}

GetHeadMethod::GetHeadMethod(const GetHeadMethod& other) : Method(other) {}

GetHeadMethod&	GetHeadMethod::operator=(const GetHeadMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
	return *this;
}

GetHeadMethod::~GetHeadMethod() {}

void	GetHeadMethod::executeMethod(int socketFd, Header& header) const {

	std::string body;


	if (this->_name == GET)
		Response::headerAndBody(socketFd, header, body);
	else if (this->_name == HEAD)
		Response::header(socketFd, header, body);

}
