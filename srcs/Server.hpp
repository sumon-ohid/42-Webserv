#pragma once

#include "Clients.hpp"
#include "Socket.hpp"
#include "Epoll.hpp"
#include <vector>

typedef std::vector<Socket> vecSocs;

class Server
{
private:
	// stores lsiten sockets
	vecSocs		_lstnSockets;
	// sotres clients' (connection sockets') fds
	Clients		_clnts;
	// handles the event monitoring
	Epoll		_epoll;
public:
	// Coplien's form
	Server();
	~Server();
	Server(const Server&);
	Server&	operator=(const Server&);

	// listen sockets
	void	createLstnSockets();
	
	// epoll
	void	callEpoll();

	// client handling
	void	addCnctSocket(int);

	// getters
	unsigned		getLstnSocketsCount() const;
	unsigned		getNumCnctSockets() const;
	const vecSocs&	getLstnSockets() const;
	const lstInt&	getCnctFds() const;
};
