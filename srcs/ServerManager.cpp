#include "ServerManager.hpp"
#include "Epoll.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include <exception>
#include <stdexcept>
#include <vector>

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

void	ServerManager::setUp(int argc, char **argv)
{
	if (argc == 2)
		_generalConfig = ServerConfig(argv[1]);

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
	std::runtime_error("couldn't create any servers");
}

void	ServerManager::runRoutine()
{
	_epoll.EpollRoutine(_servers);
}

void	ServerManager::shutdownAndEraseServer(Server& serv)
{
	vSrv::iterator it = std::find(_servers.begin(), _servers.end(), serv);
	if (it != _servers.end())
	{
		it->shutdownServer();
		_servers.erase(it);
	}
}

void	ServerManager::shutdownServer(Server& serv)
{
	vSrv::iterator it = std::find(_servers.begin(), _servers.end(), serv);
	if (it != _servers.end())
		it->shutdownServer();
}

void	ServerManager::shutdownAllServers()
{
	for (vSrv::iterator it = _servers.begin(); it != _servers.end();)
	{
		shutdownServer(*it);
		_servers.erase(it);
	}
}

void	ServerManager::closeEpoll()
{
	int	epollFd = _epoll.getFd();
	if (epollFd != -1)
		close(_epoll.getFd());
}

void	ServerManager::shutdown()
{
	std::cout << "Got to shutdown" << std::endl;
	shutdownAllServers();
	closeEpoll();
}
