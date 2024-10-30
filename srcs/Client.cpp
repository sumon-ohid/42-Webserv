#include "../includes/Client.hpp"

// ------------- Coplien's Form -------------
Client::Client() : _fd(-1), _port(-1), _lastActive(0), _numRequests(0), _server(NULL), _socket(NULL) {};
Client::Client(int fd, int port, Server *serv, Socket* socket) : _fd(fd), _port(port), _lastActive(0), _numRequests(0), _server(serv), _socket(socket) {}
Client::Client(const Client& orig) : _fd(orig._fd), _port(orig._port), _lastActive(orig._lastActive), _numRequests(orig._numRequests), _server(orig._server), _request(orig._request), _socket(orig._socket) {}
Client::~Client() {}
Client&	Client::operator=(const Client& rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_port = rhs._port;
		_request = rhs._request;
		_lastActive = rhs._lastActive;
		_numRequests = rhs._numRequests;
		_server = rhs._server;
		_socket = rhs._socket;
	}
	return (*this);
}
bool	Client::operator==(const Client& other) const
{
	return (_fd == other._fd &&
			_port == other._port &&
			_lastActive == other._lastActive &&
			_numRequests == other._lastActive &&
			_server == other._server &&
			_request == other._request &&
			_socket == other._socket);
}


void		Client::setFD(int fd)
{
	_fd = fd;
}

void		Client::setPort(int port)
{
	_port = port;
}

void		Client::setLastActive(time_t time)
{
	_lastActive = time;
}

void		Client::numRequestAdd1()
{
	_numRequests++;
}

// getters

int			Client::getFd() const
{
	return (_fd);
}

int			Client::getPort() const
{
	return (_port);
}

time_t		Client::getLastActive() const
{
	return (_lastActive);
}

unsigned	Client::getNumRequest() const
{
	return (_numRequests);
}
