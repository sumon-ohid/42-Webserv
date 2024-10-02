#pragma once

#include <sys/epoll.h>

#define	MAX_EVENTS					100000

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

	void	createEpoll(void);
	void	registerSockets(const Server&);
	void	EpollLoop(const Server&);
	void	EpollUse(const Server&);
};
