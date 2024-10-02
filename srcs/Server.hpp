#pragma once

#include "Socket.hpp"
#include "EPoll.hpp"
#include <vector>

typedef std::vector<Socket> vs;

class Server
{
private:
	vs		_sockets;
	Epoll	_epoll;
public:
	Server();
	~Server();
	Server(const Server&);
	Server&	operator=(const Server&);

	unsigned	getNumSockets(void) const;
	vs			getSockets(void) const;
};
