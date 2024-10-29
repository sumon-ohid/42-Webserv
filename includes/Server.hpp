#pragma once

#include "Client.hpp"
#include "Epoll.hpp"
#include "ServerConfig.hpp"
#include "ServerManager.hpp"
#include "Socket.hpp"
#include <list>

class Socket;

typedef std::list<Socket> lstSocs;
typedef std::map<int, Client> mpCl;

class Client;
class ServerManager;
class Epoll;

class Server
{
private:
	//
	// stores listen sockets
	lstSocs			_listenSockets;
	// stores clients
	mpCl			_clients;
	ServerConfig	_serverConfig;
	// handles the event monitoring
	// removes all client fds from epoll and _clnts and closes the fds
	void	disconnectClients(void);
	// removes all listen sockets from epoll and closes the fds
	void	disconnectLstnSockets(void);
	// DISCUSS: not sure if the shutdown should be specific to a certain address:port combination to be able to shut down a specific server
public:
	Epoll*			_epoll;

	// Coplien's form
	Server();
	Server(ServerConfig server);
	~Server();
	Server(const Server&);
	Server&	operator=(const Server&);
	bool operator==(const Server& other) const;


	// ------------- Listen Sockets -------------
	// creates a socket to listen on for all the IP, port combinations requested
	void	setUpLstnSockets(ServerManager&);
	bool	ipPortCombinationNonExistent(const std::string&, std::string&, int);

	// ------------- Client handling -------------
	// adds a client's fd to _clnts
	void	addClient(Client&);
	// removes a client's fd from _clnts
	void	removeClientFromServer(int);
	// lists all clients currently connected
	void	listClients(void) const;
	// returns true if client is connected and false if not
	bool	isClientConnected(int fd) const;

	// ------------- Server shutdown -------------
	// removes all current fds from epoll and, if it is a connection socekt, from _clnts and closes the fds
	void	shutdownServer(void);

	// Counts
	// get the number of connected listen sockets
	unsigned		listenSocketsCount() const;
	// Get the number of connected client sockets
	unsigned		CnctSocketsCount(void) const;

	// ------------- Getters -------------
	// Get a const reference to the list of listening sockets
	lstSocs&		getLstnSockets(void);
	// returns a pointer to a client if the client is found connected to the server; otherwise NULL
	Client*			getClient(int fd);

	ServerConfig	getServerConfig() const;

	Socket*			getSocket(int port);
};
