#include "Socket.hpp"
#include "Exception.hpp"
#include <cstdio>
#include <sys/socket.h>
#include <vector>

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
Socket::Socket(const Socket &orig) : _fd(orig._fd), _addrlen(orig._addrlen), _newSocket(orig._newSocket), _valread(orig._valread), _address(orig._address), _buffer(orig._buffer)
{}
Socket&	Socket::operator=(const Socket &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_port = rhs._port;
		_addrlen = rhs._addrlen;
		_newSocket = rhs._newSocket;
		_address = rhs._address;
		_valread = rhs._valread;
		_buffer = rhs._buffer;
	}
	return (*this);
}

// Functions

void	Socket::createSocket()
{
	std::cout << "Socket - create Socket" << std::endl;
	_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (_fd < 0)
		throw SocketCreationError();
	bindToSocket();
}

void	Socket::bindToSocket()
{
	std::cout << "Socket bind to socket\t" << std::endl;
	socketSetUpAddress();
	if (bind(_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw SocketBindingError();
	if (listen(_fd, SOCKET_MAX_LISTEN) < 0)
		throw SocketListenError();
	std::cout << "End of socket bind to socket" << std::endl;
}

void	Socket::socketSetUpAddress()
{
	std::cout << "Socket - set up address" << std::endl;
	_addrlen = sizeof(_address);
	_address.sin_family = AF_INET;
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port);
	std::memset(_address.sin_zero, '\0', sizeof _address.sin_zero);
	std::cout << "Socket set up at port\t" << _port << std::endl;
}

void	Socket::socketLoop()
{
	std::string hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
	while (1)
	{
		if ((_newSocket = accept(_fd, (struct sockaddr *)&_address, (socklen_t *)&_addrlen)) < 0)
			throw SocketAcceptError();
		_buffer.assign(_buffer.size(), 0);
		_valread = read(_newSocket, &_buffer[0], _buffer.size());
		if (_valread < 0)
			throw ReadError();
		std::cout << _buffer << "\n" << std::endl;
		write(_newSocket , hello.c_str() , hello.size());
        std::cout << "------------------Hello message sent-------------------" << std::endl;;
        close(_newSocket);
	}
}

const int&	Socket::getFdSocket() const
{
	return (_fd);
}

int&	Socket::getFdSocket(void)
{
	return (_fd);	
}

const int&	Socket::getPort() const
{
	return (_port);
}

socklen_t&			Socket::getAddressLen(void)
{
	return (_addrlen);
}

const socklen_t&	Socket::getAddressLen(void) const
{
	return (_addrlen);
}

const sockaddr_in&	Socket::getAddress(void) const
{
	return (_address);
}

// ostream

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc)
{
	for (std::vector<char>::const_iterator it = vc.begin(); it != vc.end(); ++it)
		os << *it;
	os << std::endl;
	return (os);
}
