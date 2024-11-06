#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"
#include "../includes/Response.hpp"
#include "../includes/Helper.hpp"

#include <iostream>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <netdb.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/types.h>
#include <utility>
#include <vector>
#include <cstring>

Epoll::Epoll() : _epollFd(-1) {}
Epoll::~Epoll() {}
Epoll::Epoll(const Epoll &orig) : _epollFd(orig._epollFd), _connSock(orig._connSock), _nfds(orig._nfds), _ev(orig._ev)
{
	for (int i = 0; i < MAX_EVENTS; ++i)
			_events[i] = orig._events[i];
}
Epoll&	Epoll::operator=(const Epoll &rhs)
{
	if (this != &rhs)
	{
		_epollFd = rhs._epollFd;
		_connSock = rhs._connSock;
		_nfds = rhs._nfds;
		_ev = rhs._ev;
		for (int i = 0; i < MAX_EVENTS; ++i)
			_events[i] = rhs._events[i];
	}
	return (*this);
}

void	Epoll::Routine(std::vector<Server> &servers)
{
	createEpoll();
	registerLstnSockets(servers);
	Monitoring(servers);
}

void	Epoll::createEpoll()
{
	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1)
		std::runtime_error("epoll - creation failed");
}

void	Epoll::registerLstnSockets(vSrv& servers)
{
	for (vSrv::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		it->_epoll = this;
		// Retrieve the list of listening sockets from the server
		const lstSocs&	sockets = it->getLstnSockets();
		// Iterate over each listening socket
		static int i = 1;
		for (lstSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
		{
			registerSocket(it->getFdSocket(), EPOLLIN);
			std::cout << BOLD BLUE "registered at port:\thttp://" << it->getIp() << ":" << it->getPort() << RESET <<  std::endl;
		}
		i++;
	}
}

bool	Epoll::registerSocket(int fd, uint32_t events)
{
	// Set the event flags to monitor for incoming connections
	_ev.events = events;  // Set event to listen for incoming data
	_ev.data.fd = fd;  // Set the file descriptor for the client socket
	// Add the socket to the epoll instance for monitoring
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &_ev) == -1)
	{
		close(fd);
		std::cerr << "socket couldn't be added to epoll" << std::endl;
		return (false);
	}
	return (true);
}

void Epoll::Monitoring(vSrv& servers)
{
	// Infinite loop to process events
	while (1)
	{
		// Wait for events on the epoll instance
		if (checkEpollWait(epoll_wait(_epollFd, _events, MAX_EVENTS, -1)) == -1)
			break;
		for (int i = 0; i < _nfds; ++i)
			// Handle events on existing client connections
			if (!cgi(_events[i].data.fd, _events[i].events) && !NewClient(servers, _events[i].data.fd))  // Check if the event corresponds to one of the listening sockets
				existingClient(servers, _events[i].events, _events[i].data.fd);
	}
}

int	Epoll::checkEpollWait(int epollWaitReturn)
{
	_nfds = epollWaitReturn;
	if (_nfds == -1)
	{
		if (errno == EINTR)
			return (-1);
		else
			throw std::runtime_error("epoll_wait failed");
	}
	return (_nfds);
}

bool	Epoll::cgi(int eventFd, uint32_t events) //have additional check here if client is still connected
{
	std::map<int, Client*>::iterator it = _mpCgiClient.find(eventFd);
	if (it == _mpCgiClient.end())
		return (false);
	Client* client = it->second;
	if (events & EPOLLERR)
		cgiErrorOrHungUp(eventFd);
	if (events & (EPOLLHUP | EPOLLRDHUP))
	{
		if (client->_isCgi && !client->_cgi.getCgiDone())
			client->_cgi.readFromChildFd(client);
	}
	if (client->_isCgi)
	{
		if (events & EPOLLIN)
		{
			client->_cgi.readFromChildFd(client);
		}	
		if (events & EPOLLOUT)
			client->_cgi.writeToChildFd(client);
	}
	// for later when reading from bigger file
	// else
	// {
	// 	if (events & EPOLLIN)
	// 		client->_request->readFile();
	// 	return (true);
	// }
	return (true);
}

void	Epoll::cgiErrorOrHungUp(int cgiFd)
{
	// std::cerr << "Error or cgi hung up on fd: " << cgiFd << std::endl;
	removeCgiClientFromEpoll(cgiFd);
}

Client*	Epoll::retrieveClient(vSrv& servers, int event_fd)
{
	for (vSrv::iterator servIt = servers.begin(); servIt != servers.end(); ++servIt)
	{
		Client* tmp = servIt->getClient(event_fd);
		if (tmp != NULL)
			return (tmp);
	}
	return (NULL);
}

bool	Epoll::NewClient(vSrv &servers, int event_fd) // possible change: implement a flag that server does not accept new connections anymore to be able to shut it down
{
	for (vSrv::iterator servIt = servers.begin();servIt != servers.end(); ++servIt)
	{
		// retrieve listening sockets
		lstSocs& sockets = servIt->getLstnSockets();
		// iterate over listening sockets
		for (lstSocs::iterator sockIt = sockets.begin(); sockIt != sockets.end(); ++sockIt)
			// Compare event_fd with the listening socket's file descriptor
			if (event_fd == sockIt->getFdSocket())
				// This is a listening socket, accept new client connection
				return (AcceptNewClient(*servIt, sockIt));
	}
	return (false);
}

bool	Epoll::AcceptNewClient(Server &serv, lstSocs::iterator& sockIt)
{
	// Get the length of the address associated with the current listening socket
	socklen_t _addrlen = sizeof(sockIt->getAddress()); //implement function in Socket: setAddrlen
	// sockIt->getAddressLen();
	// Accept a new client connection on the listening socket
	_connSock = accept4(sockIt->getFdSocket(), (struct sockaddr *) &sockIt->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (_connSock < 0)
	{
		std::cerr << gai_strerror(_connSock);
		throw std::runtime_error("Error:\taccept4 failed");
		  // Skip to the next socket if accept fails
	}
	std::cout << "New client connected: FD " << _connSock << std::endl;
	// Add the new client file descriptor to the server's list of connected clients
	if (!registerSocket(_connSock, EPOLLIN | EPOLLET))
		return (false);
	Client	tmp(_connSock, sockIt->getPort(), &serv, &(*sockIt), this);
	// addTimestamp(tmp);
	serv.addClient(tmp);
	return (true);
}

void	Epoll::existingClient(vSrv &servers, uint32_t events, int event_fd)
{
	Client* client = retrieveClient(servers, event_fd);
	if (!client)
		return (clientRetrievalError(event_fd));
	if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
		return (clientErrorOrHungUp(client));
	if (events & EPOLLIN)  // Check if the event is for reading
		client->_request.clientRequest(client);
		// should handle read events
	if (events & EPOLLOUT) // Check if the event is for writing
		clientResponse(client);
}

void	Epoll::clientRetrievalError(int event_fd)
{
	std::cerr << "Error:\tretrieving client" << std::endl;
	removeClientEpoll(event_fd);
}

void	Epoll::clientErrorOrHungUp(Client* client)
{
	// Handle error or hung up situation
	std::cerr << "Error or client hung up on fd: " << client->getFd() << std::endl;
	removeClient(client); // Remove the client from epoll and close the connection
}

void	Epoll::clientResponse(Client* client)
{
	if ((client->_isCgi && client->_request._response->getBodySize() > 0) || !client->_isCgi  || (client->_isCgi && client->_cgi.getCgiDone()))
		client->_request._response->sendResponse(client, client->getFd(), client->_request);
	if (client->_request._response->getIsFinished())
	{
		Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLIN | EPOLLET);
		client->_request.requestReset();
		client->_cgi = HandleCgi();
	}
}

void	Epoll::addCgiClientToEpollMap(int pipeFd, Client* client)
{
	std::cout << "added pipeFd:\t" << pipeFd << " to Epoll map" << std::endl;
	_mpCgiClient.insert(std::make_pair(pipeFd, client));
}

void	Epoll::removeCgiClientFromEpoll(int pipeFd)
{
	std::map<int, Client*>::iterator it = _mpCgiClient.find(pipeFd);
	_mpCgiClient.erase(it);
	removeClientEpoll(pipeFd);
	std::cout << "removeCgiClient from Epoll: " << pipeFd << std::endl;
}


void	Epoll:: removeClientEpoll(int fd)
{
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		std::cerr << "Error removing FD from epoll: " << strerror(errno) << std::endl;
	// Close the client socket
	close(fd);
}

void	Epoll::removeClientFromServer(Server* serv, int fd)
{
	serv->removeClientFromServer(fd);
}

void	Epoll::removeClient(Client* client)
{
	if (!client || !client->_server)
		return;
	removeClientEpoll(client->getFd());
	removeClientFromServer(client->_server, client->getFd());
}

int	Epoll::getFd() const
{
	return (_epollFd);
}
