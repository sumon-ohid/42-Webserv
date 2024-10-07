#include "Server.hpp"
#include "Clients.hpp"
#include "Epoll.hpp"
#include "ServerConfig.hpp"
#include <stdexcept>

// ------------- Coplien's form -------------

Server::Server() { _configFile = "None";}
Server::Server(ServerConfig server) : _server(server) {}
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

// ------------- Sockets -------------

void	Server::setUpLstnSockets()
{
	int numSockets = 3;
	int ports[] = {3000, 4000, 5000};
	for (int i = 0; i < numSockets; ++i)
	{
        // create a temporary socket instance which will listen to a specific port
		Socket	tmp(ports[i]);
		std::cout << "Server - create Sockets\t" << i << std::endl;
		tmp.setUpSocket();
        // store the socket in a vector to keep track of all listening sockets if the socket was created successfully
		if (tmp.getFdSocket() != -1)
			_lstnSockets.push_back(tmp);
	}
	// check if there is at least one listening socket
	if (_lstnSockets.empty())
		throw std::runtime_error("couldn't create any listen socket");
}

// ------------- Epoll ------------- 

void	Server::startEpollRoutine()
{
	// call epoll to set up the monitoring and enable to have clients 
    // connect to listening sockets
    _epoll.EpollRoutine(*this);
}

// ------------- Clients ------------- 

void	Server::addClientFd(int fd)
{
	_clnts.addClient(fd);
}

void	Server::removeClientFd(int fd)
{
	_clnts.removeClient(fd);
}

void	Server::listClients() const
{
	_clnts.listClients();
}

bool	Server::isClientConnected(int fd) const
{
	return (_clnts.isClientConnected(fd));
}

// ------------- Shutdown -------------

void	Server::shutdownServer()
{
	disconnectClients();
	disconnectLstnSockets();
	int	epollFd = _epoll.getFd();
	if (epollFd != -1)
		close(_epoll.getFd());
}

void	Server::disconnectClients(void)
{
	lstInt&	clientsFds = _clnts.getClientFds();
	std::cout << "disconnectClients" << std::endl;
	for (lstInt::iterator it = clientsFds.begin(); it != clientsFds.end();)
	{
		int	&fd = *(it++);
		if (fd != -1)
			_epoll.removeFdEpoll(fd);
	}
}

void	Server::disconnectLstnSockets(void)
{
	for (lstSocs::iterator it = _lstnSockets.begin(); it != _lstnSockets.end();)
	{
		if (it->getFdSocket() != -1)
			_epoll.removeFdEpoll(it->getFdSocket());
		it = _lstnSockets.erase(it);
	}
}

// ------------- Getters -------------

unsigned	Server::getLstnSocketsCount() const
{
	return (_lstnSockets.size());
}

unsigned	Server::getNumCnctSockets() const
{
	return (_clnts.getClientCount());
}

const lstSocs& Server::getLstnSockets() const
{
	return (_lstnSockets);
}

const lstInt& Server::getCnctFds() const
{
	return (_clnts.getClientFds());
}

void	Server::printLst()
{
	_clnts.listClients();
}

std::string Server::getConfigFile()
{
	return (_configFile);
}
