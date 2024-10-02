#pragma once

#include "Socket.hpp"
#include "Epoll.hpp"
#include <vector>

typedef std::vector<Socket> vecSocs;

class Server
{
private:
	vecSocs		_lstnSockets;
	vecSocs		_cnctSockets;
	Epoll		_epoll;
public:
	Server();
	~Server();
	Server(const Server&);
	Server&	operator=(const Server&);

	void	createSockets();
	void	callEpoll();

	unsigned	getNumSockets(void) const;
	const vs&	getSockets(void) const;
	vs&			getSocket(void);
};
