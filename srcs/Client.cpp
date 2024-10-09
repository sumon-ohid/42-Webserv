#include "Client.hpp"

// ------------- Coplien's Form -------------
Client::Client() : _lastActive(0), _numRequests(0), _server(NULL) {};
Client::Client(int fd, Server *serv) : _fd(fd), _lastActive(0), _numRequests(0), _server(serv) {}
Client::Client(const Client& orig) : _fd(orig._fd), _lastActive(orig._lastActive), _numRequests(orig._numRequests), _server(orig._server), _request(orig._request) {}
Client::~Client() {}
Client&	Client::operator=(const Client& rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_request = rhs._request;
		_lastActive = rhs._lastActive;
		_numRequests = rhs._numRequests;
		_server = rhs._server;
	}
	return (*this);
}
bool	Client::operator==(const Client& other) const
{
	return (_fd == other._fd &&
			_lastActive == other._lastActive &&
			_numRequests == other._lastActive &&
			_server == other._server &&
			_request == other._request);
}


void		Client::setFD(int fd)
{
	_fd = fd;
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

time_t		Client::getLastActive() const
{
	return (_lastActive);
}

unsigned	Client::getNumRequest() const
{
	return (_numRequests);
}
