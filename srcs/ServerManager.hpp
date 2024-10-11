#pragma once

#include "Epoll.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include <vector>

#define LOCATION_CONFIG_FILE "./webserv.conf"

typedef std::vector<Server>			vSrv;
typedef std::vector<ServerConfig>	vSrvConf;

class ServerManager
{
private:
	ServerConfig			_generalConfig;
	std::vector<Server>		_servers;
	Epoll					_epoll;

	// calls the config file parsing and the server setUp
	void	setUp(int argc, char **argv);
	// sets up the servers as specified in the config file
	void	setUpServers();
	// shuts down and erases all servers from the server list
	void	shutdownAllServers();
	// closes the epoll fd if not -1
	void	closeEpoll();
	// shuts down and erase from the server list a single server
	void	shutdownAndEraseServer(vSrv::iterator&);
public:
	ServerManager();
	~ServerManager();
	ServerManager(const ServerManager&);
	ServerManager&	operator=(const ServerManager&);

	// sets up the webservers and start the epoll routine
	void	runWebservs(int argc, char **argv);
	// tests if specific server is in server list and shuts down this server
	void	shutdownServer(Server&);
	// shuts down all servers and close all fds
	void	shutdown();
};
