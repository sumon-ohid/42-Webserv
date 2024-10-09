#pragma once

#include "Epoll.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include <vector>

typedef std::vector<Server>			vSrv;
typedef std::vector<ServerConfig>	vSrvConf;

class ServerManager
{
private:
	ServerConfig			_generalConfig;
	std::vector<Server>		_servers;
	Epoll					_epoll;
public:
	ServerManager();
	~ServerManager();
	ServerManager(const ServerManager&);
	ServerManager&	operator=(const ServerManager&);

	void	setUp(int argc, char **argv);
	void	setUpServers();
	void	runRoutine();
	void	shutdownServer(Server&);
	void	shutdownAndEraseServer(Server& serv);
	void	shutdownAllServers();
	void	closeEpoll();
	void	shutdown();
};
