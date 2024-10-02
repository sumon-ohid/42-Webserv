#include "EPoll.hpp"
#include <exception>
#include <sys/epoll.h>
#include <sys/socket.h>
#include "Server.hpp"

class	Epoll::EpollCreateError : public std::exception
{
public:
	const char* what() const throw() {return EPOLL_CREATE_ERROR;}
};

class	Epoll::EpollListenSocketError : public std::exception
{
public:
	const char* what() const throw() {return EPOLL_LISTEN_SOCKET_ERROR;}
};

class	Epoll::EpollWaitError : public std::exception
{
public:
	const char* what() const throw() {return EPOLL_WAIT_ERROR;}
};

Epoll::Epoll() {}
Epoll::~Epoll() {}
Epoll::Epoll(const Epoll &orig) : _fd(orig._fd), _listenSock(orig._listenSock), _connSock(orig._connSock), _nfds(orig._nfds), _ev(orig._ev)
{
	for (int i = 0; i < MAX_EVENTS; ++i)
			_events[i] = orig._events[i];
}
Epoll&	Epoll::operator=(const Epoll &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_listenSock = rhs._listenSock;
		_connSock = rhs._connSock;
		_nfds = rhs._nfds;
		_ev = rhs._ev;
		for (int i = 0; i < MAX_EVENTS; ++i)
			_events[i] = rhs._events[i];
	}
	return (*this);
}

void	Epoll::createEpoll()
{
	_fd = epoll_create1(EPOLL_CLOEXEC);
	if (_fd == -1)
		throw EpollCreateError();
}

void	Epoll::registerSockets(Server& serv)
{
	_ev.events = EPOLLIN | EPOLLOUT;
	vs	sockets = serv.getSockets();
	for (vs::iterator it = sockets.begin(); it != sockets.end(); ++it)
	{
		_ev.data.fd = it->getFdSocket();
		try
		{
			if (epoll_ctl(_fd, EPOLL_CTL_ADD, it->getFdSocket(), &_ev) == -1)
				throw EpollListenSocketError();
		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}

void	Epoll::EpollLoop(Server &serv)
{
	while (1)
	{
		_nfds = epoll_wait(_fd, _events, MAX_EVENTS, -1);
		if (_nfds == -1)
			throw EpollWaitError();
		vs	socket = serv.getSockets();
		for (int i = 0; i < _nfds; ++i)
		{
			for (vs::iterator it = socket.begin(); it != socket.end(); ++it)
			{
				if (_events[i].data.fd == it->getFdSocket())
				{
					_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &it->getAddressLen(), SOCK_NONBLOCK | SOCK_CLOEXEC);
					if (_connSock < 0)
						throw INSERT_EXCEPTION_CLASS();
					_ev.events = EPOLLIN | EPOLLOUT;
					_ev.data.fd = _connSock;
					if (epoll_ctl(_fd, EPOLL_CTL_ADD, _connSock, &_ev) == -1)
						throw EXCEPTION();
				}
				else
				{
					char buffer[1024];
					ssize_t count = read(_events[i].data.fd, buffer, sizeof(buffer));

					if (count == -1)
					{
	 					perror("read");
	 					close(_events[i].data.fd);
 					}
					else if (count == 0)
					{
	 					// Client disconnected
	 					close(_events[i].data.fd);
 					}
					else
					{
	 				// Process the data (echo back for example)
	 				write(_events[i].data.fd, buffer, count);
 					}
				}
			}
		}
	}
}
