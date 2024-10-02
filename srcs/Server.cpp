#include "Server.hpp"

Server::Server() {}
Server::~Server() {}
Server::Server(const Server &orig) : _sockets(orig._sockets), _epoll(orig._epoll) {}
Server&	Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		_sockets = rhs._sockets;
		_epoll = rhs._epoll;
	}
	return (*this);
}

unsigned	Server::getNumSockets() const
{
	return (_sockets.size());
}

