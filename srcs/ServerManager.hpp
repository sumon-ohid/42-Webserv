#pragma once

#include "Epoll.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"

class ServerManager
{
private:
	std::vector<Server>	_servers;
	ServerConfig		_generalConfig;
	Epoll				_epoll;
public:
	ServerManager();
	~ServerManager();
	ServerManager(const ServerManager&);
	ServerManager&	operator=(const ServerManager&);

	void	setUp();
	void	runRoutine();
	void	shutdown();
};
