#include "ServerManager.hpp"
#include "ServerConfig.hpp"
#include <vector>

ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}
ServerManager::ServerManager(const ServerManager& orig) : _servers(orig._servers), _generalConfig(orig._generalConfig), _epoll(orig._epoll) {}
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

void	ServerManager::setUp(int argc, char **argv)
{
	try
	{
		if (argc == 2)
		{
			_generalConfig = ServerConfig(argv[1]);
			//config.displayConfig();
		}
		server.setUpLstnSockets();
		server.startEpollRoutine();
	}
}

void	ServerManager::createServers()
{
	std::vector<ServerConfig>	servConf = _generalConfig.getServers();
	for (std::vector<ServerConfig>::iterator it = servConf.begin(); it != servConf.end(); it++)
	{
		Server	tmp = Server(*it);
	}
}
