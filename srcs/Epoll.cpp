#include "Epoll.hpp"
#include <cerrno>
#include <exception>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "Server.hpp"

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
		std::runtime_error("epoll - creation failed");
}

void	Epoll::registerSockets(const Server& serv)
{
	_ev.events = EPOLLIN | EPOLLOUT;
	vs	sockets = serv.getSockets();
	for (vs::iterator it = sockets.begin(); it != sockets.end(); ++it)
	{
		_ev.data.fd = it->getFdSocket();
		try
		{
			if (epoll_ctl(_fd, EPOLL_CTL_ADD, it->getFdSocket(), &_ev) == -1)
				throw std::runtime_error("epoll_ctl: listen_sock");
		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}

//

void Epoll::EpollLoop(const Server &serv)
{
	while (1)
	{
		_nfds = epoll_wait(_fd, _events, MAX_EVENTS, -1);
		if (_nfds == -1)
			throw std::runtime_error("epoll_wait failed");

		const vs& sockets = serv.getSockets();  // Use reference to avoid copying
		for (int i = 0; i < _nfds; ++i)
		{
			int event_fd = _events[i].data.fd;  // File descriptor for this event

			// Check if the event corresponds to one of the listening sockets
			bool is_listening_socket = false;
			for (vs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
			{
				if (event_fd == it->getFdSocket())
				{
					// This is a listening socket, accept new client
					socklen_t _addrlen = it->getAddressLen();
					_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
					if (_connSock < 0)
					{
						// perror("accept4 failed");
						continue;  // Skip this socket
					}
					std::cout << "New client connected: FD " << _connSock << std::endl;

					// Register the new client socket with EPOLLIN (ready to read)
					_ev.events = EPOLLIN;
					_ev.data.fd = _connSock;
					if (epoll_ctl(_fd, EPOLL_CTL_ADD, _connSock, &_ev) == -1)
					{
						// perror("epoll_ctl: conn_sock");
						close(_connSock);
					}
					is_listening_socket = true;
					break;  // Exit the loop as we handled this event
				}
			}

			// If it's a new client connection, skip to the next event
			if (is_listening_socket)
				continue;

			// Handle events on existing client connections
			if (_events[i].events & EPOLLIN)
			{
				// Read data from the client
				char buffer[300000] = {0};  // Zero-initialize the buffer
				ssize_t count = read(event_fd, buffer, sizeof(buffer));

				if (count == -1)
				{
					if (errno != EAGAIN && errno != EWOULDBLOCK)
					{
						// perror("read error");
						close(event_fd);
					}
					continue;
				}
				else if (count == 0)
				{
					// Client disconnected, close the socket and remove from epoll
					std::cout << "Client disconnected: FD " << event_fd << std::endl;
					close(event_fd);
					continue;
				}
				else
				{
					// Successfully read data, print it and respond
					std::cout << "Received " << count << " bytes: " << buffer << std::endl;

					std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!\n";
					ssize_t sent = write(event_fd, response.c_str(), response.size());

					if (sent == -1)
					{
						// perror("write failed");
						close(event_fd);  // Close socket on write failure
					}
					else
					{
						std::cout << "Response sent to client on FD " << event_fd << std::endl;
					}
				}
			}
		}
	}
}

void	Epoll::EpollUse(const Server& serv)
{
	createEpoll();
	registerSockets(serv);
	EpollLoop(serv);
}
