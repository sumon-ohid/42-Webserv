#include "Socket.hpp"
#include "Exception.hpp"
#include <cstdio>
#include <string>
#include <vector>

#include "./Header.hpp"

// Exceptions

class	Socket::SocketBindingError : public std::exception
{
	public:
		const char*	what() const throw() {return SOCKET_BINDING_ERROR;}
};

class	Socket::SocketListenError : public std::exception
{
	public:
		const char* what() const throw() {return SOCKET_LISTEN_ERROR;}
};

class	Socket::SocketCreationError
{
	public:
		const char* what() const throw() {return SOCKET_CREATION_ERROR;}
};

class	Socket::SocketAcceptError
{
	public:
		const char* what() const throw() {return SOCKET_ACCEPT_ERROR;}
};

// Coplien
Socket::Socket() : _port(-1), _buffer(0, 0){}
Socket::Socket(int port) : _port(port), _buffer(SOCKET_BUFFER_SIZE, 0)
{}
Socket::~Socket(){}
Socket::Socket(const Socket &orig) : _server_fd(orig._server_fd), _addrlen(orig._addrlen), _new_socket(orig._new_socket), _valread(orig._valread), _address(orig._address), _buffer(orig._buffer)
{}
Socket&	Socket::operator=(const Socket &rhs)
{
	if (this != &rhs)
	{
		_server_fd = rhs._server_fd;
		_port = rhs._port;
		_addrlen = rhs._addrlen;
		_new_socket = rhs._new_socket;
		_address = rhs._address;
		_valread = rhs._valread;
		_buffer = rhs._buffer;
	}
	return (*this);
}

// Functions

void	Socket::createSocket()
{
	_server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_server_fd < 0)
		throw SocketCreationError();
	bindToSocket();
}

void	Socket::bindToSocket()
{
	socketSetUpAddress();
	if (bind(_server_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw SocketBindingError();
	if (listen(_server_fd, SOCKET_MAX_LISTEN) < 0)
		throw SocketListenError();
	socketLoop();
}

void	Socket::socketSetUpAddress()
{
	_addrlen = sizeof(_address);
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	std::memset(_address.sin_zero, '\0', sizeof _address.sin_zero);
}

void	Socket::socketLoop()
{
	std::string hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 13\n\nHello world!\n";
	while (1)
	{
		Header header;
		if ((_new_socket = accept(_server_fd, (struct sockaddr *)&_address, (socklen_t *)&_addrlen)) < 0)
			throw SocketAcceptError();
		_buffer.assign(_buffer.size(), 0);
		_valread = read(_new_socket, &_buffer[0], _buffer.size());
		if (_valread < 0)
			throw ReadError();
		_buffer.resize(_valread);
		while (header.getReadingFinished()) {
			if(header.getFirstLineChecked()) {

			} else {
				header.checkFirstLine(_buffer);
			}

		}
		std::string test(_buffer.begin(), _buffer.end());
 		std::cout << "$" << test << "$ " << _buffer.size() << std::endl;
		write(_new_socket , hello.c_str() , hello.size());
    	std::cout << "------------------Hello message sent-------------------" << std::endl;;
    	close(_new_socket);
	}
}

// ostream

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc)
{
	for (std::vector<char>::const_iterator it = vc.begin(); it != vc.end(); ++it)
		os << *it;
	os << std::endl;
	return (os);
}
