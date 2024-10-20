#include "ServerManager.hpp"
#include "ServerConfig.hpp"
#include "Epoll.hpp"
#include "Server.hpp"

#include <exception>
#include <stdexcept>
#include <vector>
#include <algorithm>

ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}
ServerManager::ServerManager(const ServerManager& orig) : _generalConfig(orig._generalConfig), _servers(orig._servers), _epoll(orig._epoll) {}
ServerManager&	ServerManager::operator=(const ServerManager& rhs)
{
	if (this != &rhs)
	{
		_servers = rhs._servers;
		_generalConfig = rhs._generalConfig;
		_epoll = rhs._epoll;
	}
	return (*this);
}

void	ServerManager::runWebservs(int argc, char **argv)
{
	setUp(argc, argv);
	_epoll.Routine(_servers);
}

void	ServerManager::setUp(int argc, char **argv)
{
	if (argc == 2)
		_generalConfig = ServerConfig(argv[1]);
	else
	 	_generalConfig = ServerConfig(LOCATION_CONFIG_FILE);
	//-- Display the config for debugging
	//_generalConfig.displayConfig();

	setUpServers();
}

void	ServerManager::setUpServers()
{
	vSrvConf	servConf = _generalConfig.getServers();
	for (vSrvConf::iterator it = servConf.begin(); it != servConf.end(); it++)
	{
		Server	tmp = Server(*it);
		try
		{
			tmp.setUpLstnSockets();
			_servers.push_back(tmp);
		}
		catch (std::exception &e)
		{
			std::cout << "Error:\t" << e.what() << std::endl;
		}
	}
	if (_servers.empty())
		throw std::runtime_error("couldn't create any servers");
}

void	ServerManager::shutdownAndEraseServer(vSrv::iterator &servIt)
{
	servIt->shutdownServer();
	_servers.erase(servIt);
}

void	ServerManager::shutdownServer(Server& serv)
{
	vSrv::iterator servIt = std::find(_servers.begin(), _servers.end(), serv);
	if (servIt != _servers.end())
		shutdownAndEraseServer(servIt);
}

void	ServerManager::shutdownAllServers()
{
	for (vSrv::iterator servIt = _servers.begin(); servIt != _servers.end();)
		shutdownAndEraseServer(servIt);
}

void	ServerManager::closeEpoll()
{
	int	epollFd = _epoll.getFd();
	if (epollFd != -1)
		close(_epoll.getFd());
}

void	ServerManager::shutdown()
{
	shutdownAllServers();
	closeEpoll();
	std::cout << "\nAll servers shut down" << std::endl;
}
