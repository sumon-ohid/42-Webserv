#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Epoll.hpp"
#include "../includes/ServerConfig.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

// ------------- Coplien's form -------------

Server::Server() : _configFile("none"), _epoll(NULL) {}
Server::Server(ServerConfig conf) : _serverConfig(conf), _epoll(NULL) {}
Server::~Server() {}
Server::Server(const Server &orig) : _listenSockets(orig._listenSockets), _clients(orig._clients), _configFile(orig._configFile), _serverConfig(orig._serverConfig), _epoll((orig._epoll)) {}
Server&	Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		_listenSockets = rhs._listenSockets;
		_clients = rhs._clients;
		_configFile = rhs._configFile;
		_serverConfig = rhs._serverConfig;
		_epoll = rhs._epoll;
	}
	return (*this);
}

bool Server::operator==(const Server& other) const
{
	return (_listenSockets == other._listenSockets &&
			_clients == other._clients &&
			_configFile == other._configFile &&
			_serverConfig == other._serverConfig &&
			_epoll == other._epoll);
}
// ------------- Sockets -------------

void	Server::setUpLstnSockets()
{
	std::vector<int> ports = this->_serverConfig.getListenPorts();
	for (size_t i = 0; i < ports.size(); ++i)
	{
		int port = ports[i];
        // create a temporary socket instance which will listen to a specific port
		Socket	tmp(port);
		tmp.setUpSocket();
        // store the socket in a vector to keep track of all listening sockets if the socket was created successfully
		if (tmp.getFdSocket() != -1)
			_listenSockets.push_back(tmp);
	}
	// check if there is at least one listening socket
	if (_listenSockets.empty())
		throw std::runtime_error("couldn't create any listen socket");
}

// ------------- Clients -------------

void	Server::addClient(Client& cl)
{
	_clients.insert(std::make_pair(cl.getFd(), cl));
}

void	Server::removeClientFromServer(int fd)
{
	mpCl::iterator it = _clients.find(fd);
	if (it != _clients.end())
		_clients.erase(it);
}


void	Server::listClients() const
{
	std::cout << "Clients connected to server listening at ports (insert ports)" << std::endl;
	for (mpCl::const_iterator it = _clients.begin(); it != _clients.end();)
	{
		std::cout << it->second.getFd();
		if (++it != _clients.end())
			std::cout << ", ";
	}
	if (_clients.empty())
		std::cout << "No clients connected" << std::endl;
	else
		std::cout << std::endl;
}

bool	Server::isClientConnected(int fd) const
{
	mpCl::const_iterator it = _clients.find(fd);
	if (it != _clients.end())
		return (true);
	return (false);
}

// ------------- Shutdown -------------

void	Server::shutdownServer()
{
	disconnectClients();
	disconnectLstnSockets();
}

void	Server::disconnectClients(void)
{
	for (mpCl::iterator it = _clients.begin(); it != _clients.end();)
	{
		int	fd = (it++)->second.getFd();
		if (fd != -1)
			_epoll->removeClientEpoll(fd);
	}
	_clients.clear();
}

void	Server::disconnectLstnSockets(void)
{
	for (lstSocs::iterator it = _listenSockets.begin(); it != _listenSockets.end();)
	{
		if (it->getFdSocket() != -1)
			_epoll->removeClientEpoll(it->getFdSocket());
		it = _listenSockets.erase(it);
	}
}

// ------------- Counts -------------

unsigned	Server::listenSocketsCount() const
{
	return (_listenSockets.size());
}

unsigned	Server::CnctSocketsCount() const
{
	return (_clients.size());

// ------------- Getters -------------

}

lstSocs& Server::getLstnSockets()
{
	return (_listenSockets);
}

Client*	Server::getClient(int fd)
{
	mpCl::iterator it = _clients.find(fd);
	if (it != _clients.end())
		return (&it->second);
	return (NULL);
}
