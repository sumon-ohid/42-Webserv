#include "../includes/ServerManager.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"

#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>
#include <algorithm>

ServerManager::ServerManager() {}
ServerManager::~ServerManager() {}
ServerManager::ServerManager(const ServerManager& orig) : _generalConfig(orig._generalConfig), _servers(orig._servers), _ipPortSocket(orig._ipPortSocket), _epoll(orig._epoll) {}
ServerManager&	ServerManager::operator=(const ServerManager& rhs)
{
	if (this != &rhs)
	{
		_servers = rhs._servers;
		_generalConfig = rhs._generalConfig;
		_ipPortSocket = rhs._ipPortSocket;
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
			tmp.setUpLstnSockets(*this);
			_servers.push_back(tmp);
		}
		catch (std::exception &e)
		{
			std::cout << "Error:\t" << e.what() << std::endl;
		}
	}
	// check if there is at least one listening socket
	if (_servers.empty())
		throw std::runtime_error("couldn't create any servers");
}

void	ServerManager::printConfigs()
{
	static int i = 1;
	for (vSrv::iterator servIt = _servers.begin(); servIt != _servers.end(); ++servIt)
	{
		std::cout << "Printing config files after initializing server:\t" << i << std::endl;
		lstSocs&	sockets = servIt->getLstnSockets();
		for (lstSocs::iterator it = sockets.begin(); it != sockets.end(); ++it)
		{
			std::cout << "socket at Ip " << it->getIp() << " and port " << it->getPort() << " and a config size:\t" << it->getConfigSize() << std::endl;
			for (mHstConfs::iterator ConfigIt = it->_configs.begin(); ConfigIt != it->_configs.end(); ++ConfigIt)
				std::cout << "hostname:" << ConfigIt->first <<  "\tthere is a config file" << std::endl;
		}
		i++;
	}
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

bool	ServerManager::ipPortCombinationNonExistent(const std::string& hostname, std::string& IpHost, int port, ServerConfig servConf)
{
	for (vSrv::iterator servIt = _servers.begin(); servIt != _servers.end(); ++servIt)
	{
		lstSocs& sockets = servIt->getLstnSockets();
		for (lstSocs::iterator socIt = sockets.begin(); socIt != sockets.end(); ++socIt)
		{
			if (IpHost == socIt->getIp() && socIt->getPort() == port)
			{
				socIt->addConfig(hostname, servConf);
				return (false);
			}
		}
	}
	return (true);
}

void	ServerManager::addNewSocketIpCombination(Socket &socket, std::string& hostIp)
{
	mpIpPortSocket::iterator it = _ipPortSocket.find(std::make_pair(hostIp, socket.getPort()));
	if (it != _ipPortSocket.end())
		return;
	else
		_ipPortSocket.insert(std::make_pair(std::make_pair(hostIp, socket.getPort()), socket));
}
