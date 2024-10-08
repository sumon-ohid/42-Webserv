#pragma once

#include "Client.hpp"
#include "Socket.hpp"
#include "Epoll.hpp"
#include <list>

typedef std::list<Socket> lstSocs;
typedef std::list<Client> lstClients;

class Server
{
private:
	// stores lsiten sockets
	lstSocs			_listenSockets;
	// sotres clients' (connection sockets') fds
	lstClients		_clients;
	// handles the event monitoring
	Epoll			_epoll;
public:
	// Coplien's form

	Server();
	~Server();
	Server(const Server&);
	Server&	operator=(const Server&);

	// listen sockets

	// creates a socket to listen on for all the IP, port combinations requested
	void	setUpLstnSockets();

	void	startServer();
	
	// epoll
	// initializes the epoll routine
	void	startEpollRoutine();

	// client handling

	// adds a client's fd to _clnts
	void	addClientFd(int);
	// removes a client's fd from _clnts
	void	removeClientFd(int);
	// lists all clients currently connected
	void	listClients(void) const;
	// returns true if client is connected and false if not
	bool	isClientConnected(int fd) const;

	void	printLst();

	// server shutdown
	// removes all current fds from epoll and, if it is a connection socekt, from _clnts and closes the fds
	void	shutdownServer(void);
	// removes all client fds from epoll and _clnts and closes the fds
	void	disconnectClients(void);
	// removes all listen sockets from epoll and closes the fds
	void	disconnectLstnSockets(void);
	// DISCUSS: not sure if the shutdown should be specific to a certain address:port combination to be able to shut down a specific server

	// Getters


	unsigned		listenSocketsCount() const;
	// Get the number of connected client sockets
	unsigned		CnctSocketsCount(void) const;
	// Get a const reference to the list of listening sockets
	const lstSocs&	getLstnSockets(void) const;

};
