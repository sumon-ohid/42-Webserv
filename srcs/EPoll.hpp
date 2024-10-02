#pragma once

#include <sys/epoll.h>

#define	EPOLL_CREATE_ERROR			"epoll - creation failed"
#define	EPOLL_LISTEN_SOCKET_ERROR	"epoll_ctl: listen_sock"
#define	EPOLL_WAIT_ERROR			"epoll_wait"
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
	void	registerSockets(Server&);
	void	EpollLoop(Server&);

	class	EpollCreateError;
	class	EpollListenSocketError;
	class	EpollWaitError;
};
