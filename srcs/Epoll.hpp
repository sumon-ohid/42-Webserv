#pragma once

#include <sys/epoll.h>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <list>
#include "Socket.hpp"
#include "./Request.hpp"
#include "Response.hpp"

#define	MAX_EVENTS					100000

typedef std::list<Socket>	lstSocs;
typedef std::vector<int> 	vecInt;

class Server;

class Epoll
{
private:
	int					_epollFd;
	int					_connSock;
	int					_nfds;
	struct epoll_event _ev;
	struct epoll_event	_events[MAX_EVENTS];
public:
	Epoll();
	~Epoll();
	Epoll(const Epoll &);
	Epoll&	operator=(const Epoll&);

    // Initializes the epoll file descriptor with EPOLL_CLOEXEC.
	void	createEpoll(void);
	/**
 	* Registers listening sockets with the epoll instance.
 	* Retrieves the list of sockets from the Server and sets them for EPOLLIN monitoring.
 	*/
	void	registerLstnSockets(const Server&);
	/**
	 * Registers a socket for monitoring with epoll.
	 * Sets the event flag to EPOLLIN for incoming data; closes the socket if registration fails.
	 */
	void	registerSocket(int fd);
	/**
	 * Monitors events on the epoll instance in an infinite loop.
	 * Processes new client connections and events on existing clients.
	 *
	 * @throws std::runtime_error If epoll_wait fails.
	 */
	void	EpollMonitoring(Server&);
	/**
 	* Checks if the given file descriptor corresponds to a listening socket
 	* and attempts to accept a new client connection if so.
 	* Returns true if a new client connection was accepted, otherwise false.
 	*/
	bool	EpollNewClient(Server&, const int&); // DISCUSS: possible change: implement a flag that server does not accept new connections anymore to be able to shut it down
	/**
	 * Accepts a new client connection on the specified listening socket.
	 * Returns true if the new client was successfully accepted, otherwise false.
	 */
	bool	EpollAcceptNewClient(Server&, const lstSocs::const_iterator&);
	/**
	 * Handles communication with an existing client by reading data from the
	 * specified file descriptor. Responds to the client with a simple message
	 * and manages socket state on errors or disconnections.
	 * Returns 0 on success, -1 on errors or client disconnection.
	 */
	int		EpollExistingClient(Server&, const int&);

	// DISCUSS: should be handled in Request
	void	validRequest(std::vector<char>, ssize_t, Request&);
	int		invalidRequest(Server&, const int&);
	int		emptyRequest(Server&, const int&);

	/**
	 * Initializes the epoll instance, registers listening sockets,
	 * and enters the monitoring loop to handle incoming events.
	 */
	void	EpollRoutine(Server&);
	// removes the fd from epoll and closes the fd
	void	removeFdEpoll(int);
	// removes the fd from the clients
	void	removeFdClients(Server&, int);
	// removes fd from clients and from epoll and closes it
	void	removeFd(Server&, int);

	int		getFd(void) const;
};
