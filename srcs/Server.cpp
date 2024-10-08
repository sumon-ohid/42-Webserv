#include "Server.hpp"
#include "Client.hpp"
#include "Epoll.hpp"
#include <stdexcept>

// ------------- Coplien's form -------------

Server::Server() {}
Server::~Server() {}
Server::Server(const Server &orig) : _listenSockets(orig._listenSockets), _clients(orig._clients), _epoll(orig._epoll) {}
Server&	Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		_listenSockets = rhs._listenSockets;
		_clients = rhs._clients;
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
			_listenSockets.push_back(tmp);
	}
	// check if there is at least one listening socket
	if (_listenSockets.empty())
		throw std::runtime_error("couldn't create any listen socket");
}

// ------------- Epoll -------------

void	Server::startServer()
{
	setUpLstnSockets();
	_epoll.EpollRoutine(*this);
}

void	Server::startEpollRoutine()
{
	// call epoll to set up the monitoring and enable to have clients
    // connect to listening sockets
    _epoll.EpollRoutine(*this);
}

// ------------- Clients -------------

void	Server::addClient(int fd)
{
	_clients.push_back(Client(fd));
}

void	Server::removeClientFd(int fd)
{
	_clients.removeClient(fd);
}

void	Server::listClients() const
{
	_clients.listClients();
}

bool	Server::isClientConnected(int fd) const
{
	return (_clients.isClientConnected(fd));
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
	std::cout << "disconnectClients" << std::endl;
	for (lstClients::iterator it = _clients.begin(); it != _clients.end();)
	{
		int	fd = (it++)->getFd();
		if (fd != -1)
			_epoll.removeFdEpoll(fd);
	}
}

void	Server::disconnectLstnSockets(void)
{
	for (lstSocs::iterator it = _listenSockets.begin(); it != _listenSockets.end();)
	{
		if (it->getFdSocket() != -1)
			_epoll.removeFdEpoll(it->getFdSocket());
		it = _listenSockets.erase(it);
	}
}

// ------------- Getters -------------

unsigned	Server::listenSocketsCount() const
{
	return (_listenSockets.size());
}

unsigned	Server::CnctSocketsCount() const
{
	return (_clients.size());
}

const lstSocs& Server::getLstnSockets() const
{
	return (_listenSockets);
}
