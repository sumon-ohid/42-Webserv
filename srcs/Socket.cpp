#include "Socket.hpp"
#include <exception>

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

void	Socket::setUpSocket()
{
	try
	{
		createSocket();
		socketSetUpAddress();
		bindToSocketAndListen();
	}
	catch (std::exception &e)
	{
		if (_fd != -1)
		{
			close (_fd);
			_fd = -1;
		}
		std::cerr << "Couldn't create a socket that listens at port:\t" << _port << std::endl;
	}
}

void	Socket::createSocket()
{
	std::cout << "Socket - create Socket" << std::endl;
    // Creates a file descriptor (fd) for a socket, specifying the address family (IPv4),
    // the socket type (SOCK_STREAM for TCP), and setting the socket to non-blocking mode
    // (SOCK_NONBLOCK) while using the default protocol (0).
	_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    // if there was an error creating the socket
	if (_fd < 0)
		throw std::runtime_error("socket - could not create socket");
}


void	Socket::bindToSocketAndListen()
{
	std::cout << "Socket bind to socket\t" << std::endl;
	int	opt = 1;
		/// Enable SO_REUSEADDR (set opt = 1) to allow binding to a port in TIME_WAIT state,
		// facilitating quick restarts of the server without waiting for the socket to be released.
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			throw std::runtime_error("socket - could not set SO_REUSEADDR option");
    // binds the socket file descriptor to the specified port and IP address
	if (bind(_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
		throw std::runtime_error("socket - binding to socket failed");
	// Instructs the socket to listen for incoming connection requests, 
    // allowing up to SOCKET_MAX_LISTEN connections to be queued.
    if (listen(_fd, SOCKET_MAX_LISTEN) < 0)
		throw std::runtime_error("socket - listen failed");
	std::cout << "End of socket bind to socket" << std::endl;
}

void	Socket::socketSetUpAddress()
{
	std::cout << "Socket - set up address" << std::endl;
    // total size of sockaddr_in structure; later indicates how many bytes
    // should be read/written
	_addrlen = sizeof(_address);
	std::memset(&_address, 0, sizeof(_address)); // Clear the whole structure
	_address.sin_family = AF_INET; // specifies communication domain (here: IPv4)
    // specifies the IP address that the socket should listen to;
    // INADDR_ANY: binds the socket to all available interfaces
    // (is a constant equal to zero);
    // to bind to specific IP address (e.g., localhost: 
    // _address.sin_addr.s_addr = inet_addr("127.0.0.1");)
	_address.sin_addr.s_addr = INADDR_ANY;
	_address.sin_port = htons(_port); // sets the port number
    // clears or initializes the sin_zero field which is used as padding
    // to ensure that the size of the sockaddr_in structure
    // is the same as the sockaddr structure.
	std::memset(_address.sin_zero, '\0', sizeof _address.sin_zero);
	std::cout << "Address set up for port:\t" << _port << std::endl;
}

void	Socket::socketLoop()
{
	std::string hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
	while (1)
	{
		if ((_newSocket = accept(_fd, (struct sockaddr *)&_address, (socklen_t *)&_addrlen)) < 0)
			throw std::runtime_error("socket - accept failed");
		_buffer.assign(_buffer.size(), 0);
		_valread = read(_newSocket, &_buffer[0], _buffer.size());
		if (_valread < 0)
			throw std::runtime_error("read error");
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
