#include "Server.hpp"
#include "Clients.hpp"
#include "Epoll.hpp"

// ------------- Coplien's form -------------

Server::Server() {}
Server::~Server() {}
Server::Server(const Server &orig) : _lstnSockets(orig._lstnSockets), _clnts(orig._clnts), _epoll(orig._epoll) {}
Server&	Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		_lstnSockets = rhs._lstnSockets;
		_clnts = rhs._clnts;
		_epoll = rhs._epoll;
	}
	return (*this);
}

// ------------- handle connections -------------

void	Server::createLstnSockets()
{
	int numSockets = 3;
	int ports[] = {3000, 4000, 5000};
	for (int i = 0; i < numSockets; ++i)
	{
        // create a temporary socket instance which will listen to a specific port
		Socket	tmp(ports[i]);
		std::cout << "Server - create Sockets\t" << i << std::endl;
		tmp.createSocket();
        // store the socket in a vector to keep track of all listening sockets
		_lstnSockets.push_back(tmp);
	}
}

void	Server::addCnctSocket(int fd)
{
	_clnts.addClient(fd);
}

void	Server::callEpoll()
{
	// call epoll to set up the monitoring and enable to have clients 
    // connect to listening sockets
    _epoll.EpollRoutine(*this);
}

// ------------- getters -------------

// Get the number of listening sockets
unsigned	Server::getLstnSocketsCount() const
{
	return (_lstnSockets.size());
}

// Get the number of connected client sockets
unsigned	Server::getNumCnctSockets() const
{
	return (_clnts.getClientCount());
}

// Get a const reference to the list of listening sockets
const vecSocs& Server::getLstnSockets() const
{
	return (_lstnSockets);
}

// Get a const reference to the list of connected sockets (client FDs)
const lstInt& Server::getCnctFds() const
{
	return (_clnts.getClientFds());
}