#include "../includes/Server.hpp"
#include "../includes/Client.hpp"
#include "../includes/Epoll.hpp"
#include "../includes/ServerConfig.hpp"

#include <cstddef>
#include <exception>
#include <netdb.h>
#include <utility>
#include <vector>
#include <cerrno>

// ------------- Coplien's form -------------

Server::Server() : _epoll(NULL) {}
Server::Server(ServerConfig conf) : _serverConfig(conf), _epoll(NULL) {}
Server::~Server() {}
Server::Server(const Server &orig) : _listenSockets(orig._listenSockets), _clients(orig._clients), _serverConfig(orig._serverConfig), _epoll((orig._epoll)) {}
Server&	Server::operator=(const Server &rhs)
{
	if (this != &rhs)
	{
		_listenSockets = rhs._listenSockets;
		_clients = rhs._clients;
		_serverConfig = rhs._serverConfig;
		_epoll = rhs._epoll;
	}
	return (*this);
}

bool Server::operator==(const Server& other) const
{
	return (_listenSockets == other._listenSockets &&
			_clients == other._clients &&
			_serverConfig == other._serverConfig &&
			_epoll == other._epoll);
}
// ------------- Sockets -------------

void	Server::setUpLstnSockets(ServerManager& sm)
{
	std::vector<std::string> hostnames = _serverConfig.getServerNames();
	for (std::vector<std::string>::iterator hostname = hostnames.begin(); hostname != hostnames.end(); ++hostname)
		createPorts(sm, *hostname);
	if (hostnames.empty())
		createPorts(sm);
}

void	Server::createPorts(ServerManager& sm, const std::string& hostname)
{
	std::vector<int> ports = _serverConfig.getListenPorts();
	for (size_t i = 0; i < ports.size(); ++i)
		if (createSockets(sm, hostname, ports[i]))
			break;
	if (ports.empty())
		createSockets(sm, hostname);
}

bool	Server::createSockets(ServerManager& sm, const std::string& hostname, int port)
{
	Socket	tmp(port);
	try
	{
		tmp.setUpSocket(hostname, *this, sm);
		if (tmp.getFdSocket() != -1)
			_listenSockets.push_back(tmp);
	}
	catch (std::exception &e)
	{
		if (tmp.getFdSocket() != -1)
			close (tmp.getFdSocket());
		std::cerr << e.what() << std::endl;
		std::cerr << "Couldn't create a socket that listens at host " << hostname << " at port:\t" << port << std::endl;
		if (std::string(e.what()) == "Name or service not known")
			return (false);
	}
	return (true);
}


bool	Server::ipPortCombinationNonExistent(const std::string& hostname, std::string& IpHost, int port)
{
	for (lstSocs::iterator socIt = _listenSockets.begin(); socIt != _listenSockets.end(); ++socIt)
	{
		if (socIt->getIp() == IpHost && socIt->getPort() == port)
		{
			socIt->addConfig(hostname, _serverConfig);
			return (false);
		}
	}
	return (true);
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

Socket*	Server::getSocket(int port)
{
	for (lstSocs::iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it)
		if (port == it->getPort())
			return (&(*it));
	return (NULL);
}

ServerConfig	Server::getServerConfig() const
{
	return (_serverConfig);
}
