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

const vs&	Server::getSockets() const
{
	return (_sockets);
}

vs&	Server::getSocket()
{
	return (_sockets);
}

void	Server::createSockets()
{
	int numSockets = 3;
	int ports[] = {3000, 4000, 5000};
	for (int i = 0; i < numSockets; ++i)
	{
		Socket	tmp(ports[i]);
		std::cout << "Server - create Sockets\t" << i << std::endl;
		tmp.createSocket();
		_sockets.push_back(tmp);
	}
}

void	Server::callEpoll()
{
	_epoll.EpollUse(*this);
}
