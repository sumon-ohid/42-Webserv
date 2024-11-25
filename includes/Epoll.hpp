#pragma once

#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <list>
#include <vector>

#include "Client.hpp"
#include "./Request.hpp"

// Forward declaration of Server
class Server;
class Socket;

#define MAX_EVENTS 100000

typedef std::list<Socket> lstSocs;
typedef std::vector<int> vecInt;
typedef std::vector<Server> vSrv;
typedef std::pair<Server, Client> prSerCl;

class Epoll
{
private:
	int						_epollFd;
	int						_connSock;
	int						_nfds;
	struct epoll_event 		_ev;
	struct epoll_event		_events[MAX_EVENTS];
	std::map<int, Client*>	_mpClients;
	std::list<Client*>		_lstIoClients;

	// ------------- Epoll setup -------------
    // Initializes the epoll file descriptor with EPOLL_CLOEXEC.
	void	createEpoll(void);
	/**
 	* Registers listening sockets with the epoll instance.
 	* Retrieves the list of sockets from the Server and sets them for EPOLLIN monitoring.
 	*/
	void	registerLstnSockets(vSrv&);
	/**
	 * Monitors events on the epoll instance in an infinite loop.
	 * Processes new client connections and events on existing clients.
	 *
	 * @throws std::runtime_error If epoll_wait fails.
	 */
	// ------------- Monitoring -------------
	void	monitoring(vSrv&);
	/**
 	* Checks if the given file descriptor corresponds to a listening socket
 	* and attempts to accept a new client connection if so.
 	* Returns true if a new client connection was accepted, otherwise false.
 	*/
	int		checkEpollWait(int);

	bool	cgi(int eventFd, uint32_t events);

	void	cgiErrorOrHungUp(int cgiFd);
	// ------------- Client Handling -------------
	bool	newClient(vSrv&, int); // DISCUSS: possible change: implement a flag that server does not accept new connections anymore to be able to shut it down
	/**
	 * Accepts a new client connection on the specified listening socket.
	 * Returns true if the new client was successfully accepted, otherwise false.
	 */
	bool	acceptNewClient(Server&, lstSocs::iterator&);
	// check how the event should be handled for an existing client
	bool	existingClient(int, uint32_t);
	void	handleCgiClient(Client*, int, uint32_t);
	void	endCgi(Client*);
	void	handleRegularClient(Client*, uint32_t);

	// returns the client corresponding to the fd passed
	Client*	retrieveClient(vSrv&, int);
	// outputs the corresponding error message and removes the client from epoll
	void	clientRetrievalError(int);
	// outputs the corresponding error message and removes the client from epoll and the corresponding server
	void	clientHungUp(Client*);
	void	clientError(Client*);
	/**
	 * Handles communication with an existing client by reading data from the
	 * specified file descriptor. Responds to the client with a simple message
	 * and manages socket state on errors or disconnections.
	 * Returns 0 on success, -1 on errors or client disconnection.
	 */
	// int		clientRequest(Client*);
	// DISCUSS: should be handled in Request

	void	ioFiles();

	void	clientResponse(Client*);

	void	checkTimeouts();
	// ------------- Cleanup -------------
	// removes the fd from the clients
	void	removeClientFromServer(Server*, int);
public:
	Epoll();
	~Epoll();
	Epoll(const Epoll &);
	Epoll&	operator=(const Epoll&);

	/**
	 * Registers a socket for monitoring with epoll.
	 * Sets the event flag to EPOLLIN for incoming data; closes the socket if registration fails.
	 */
	bool	registerSocket(int, uint32_t);

	// ------------- Routine -------------
	/**
	 * Initializes the epoll instance, registers listening sockets,
	 * and enters the monitoring loop to handle incoming events.
	 */
	void	routine(vSrv&);

	void	addCgiClientToEpollMap(int, Client*);
	void	removeCgiClientFromEpoll(int);

	// ------------- Cleanup -------------
	// removes the fd from epoll and closes the fd
	void	removeClientEpoll(int);
	// removes fd from clients and from epoll and closes it
	void	removeClient(Client*);

	void	addClientIo(Client*, std::string mode);
	void	removeClientIo(Client*);

	// ------------- Getters -------------
	// returns the epoll fd
	int		getFd(void) const;

	bool	is_fd_valid(int fd);
};
