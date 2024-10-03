#pragma once

#include <sys/epoll.h>
#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "Socket.hpp"

#define	MAX_EVENTS					100000

typedef std::vector<Socket> vecSocs;
typedef std::vector<int> 	vecInt;

class Server;


class Epoll
{
private:
	int					_fd;
	int					_listenSock;
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
    *
    * - Configures event settings for EPOLLIN (to monitor incoming connections).
    * - Adds each listening socket from the server to the epoll instance for monitoring.
    */
	void	registerLstnSockets(const Server&);

	void	EpollMonitoring(const Server&);

    void    EpollEventMonitoring(const Server&);

	bool	EpollNewClient(const Server&, const int&);

	bool	EpollAcceptNewClient(const Server&, const vecSocs::const_iterator&);

	int		EpollExistingClient(const int&);

	void	EpollRoutine(const Server&);
};
