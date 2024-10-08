#include "Client.hpp"

// ------------- Coplien's Form -------------

Client::Client() : _fd(-1) {}
Client::Client(int fd) : _fd(fd) {}
Client::~Client() {}
Client::Client(const Client& orig) : _fd(orig._fd), _request(orig._request), _last_active(orig._last_active), _numRequests(orig._numRequests) {}
Client&	Client::operator=(const Client& rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_request = rhs._request;
		_last_active = rhs._last_active;
		_numRequests = rhs._numRequests;
	}
	return (*this);
}

// ------------- Client Management -------------

// void Client::addClient(int &fd)
// {
//     _clnts.push_back(fd);
// }

// void Client::removeClient(int fd)
// {
// 	_clnts.remove(fd); // Removes all occurrences of fd
// }

// void Client:: listClients() const
// {
// 	for (lstInt::const_iterator it = _clnts.begin(); it != _clnts.end(); ++it)
// 		std::cout << "FD: " << *it << std::endl;
// }

// bool Client::isClientConnected(int fd) const
// {
// 	return (std::find(_clnts.begin(), _clnts.end(), fd) != _clnts.end());
// }

// // ------------- getters -------------

// const lstInt&	Client::getClientFds() const
// {
// 	return (_clnts);
// }

// size_t			Client::getClientCount() const
// {
// 	return (_clnts.size());
// }

// lstInt&	Client::getClientFds()
// {
// 	return (_clnts);
// }


void		Client::setFD(int fd)
{
	_fd = fd;
}

void		Client::setLastActive(time_t time)
{
	_last_active = time;
}

void		Client::numRequestAdd1()
{
	_numRequests++;
}

// getters

int			Client::getFd()
{
	return (_fd);
}

time_t		Client::getLastActive()
{
	return (_last_active);
}

unsigned	Client::getNumRequest()
{
	return (_numRequests);
}

std::string	Client::getMethodName()
{
	return (_request.getMethodName());
}

std::string	Client::getMethodPath()
{
	return (_request.getMethodName());
}

std::string	Client::getMethodProtocol()
{
	return (_request.getMethodPath());
}

Request&	Client::getRequest()
{
	return (_request);
}
