#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"
#include "../includes/Response.hpp"
#include "../includes/Helper.hpp"

#include <fcntl.h>
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
Epoll::Epoll(const Epoll &orig) : _epollFd(orig._epollFd), _connSock(orig._connSock), _nfds(orig._nfds), _ev(orig._ev), _lstIoClients(orig._lstIoClients)
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
	std::memset(&_ev, 0, sizeof(_ev));  // Zero-initialize the epoll_event structure
	// Set the event flags to monitor for incoming connections
	_ev.events = events;  // Set event to listen for incoming data
	_ev.data.fd = fd;  // Set the file descriptor for the client socket
	// Add the socket to the epoll instance for monitoring
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &_ev) == -1)
    {
        std::cerr << "Error adding socket to epoll: " << strerror(errno) << std::endl;
        std::cerr << "Permission denied: " << (errno == EPERM ? "EPOLL_CTL_ADD" : "") << std::endl;
        std::cerr << "File descriptor: " << fd << std::endl;
        close(fd);
        return false;
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
		IOFiles();
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

bool	Epoll::cgi(int eventFd, uint32_t events)
{
	std::map<int, Client*>::iterator it = _mpCgiClient.find(eventFd);
	if (it == _mpCgiClient.end())
		return (false);
	Client* client = it->second;
	if (events & (EPOLLHUP | EPOLLRDHUP))
	{
		if (client->_isCgi && !client->_cgi.getCgiDone())
			client->_io.readFromChildFd(client);
		else
			removeClientIo(client);
	}
	if (events & EPOLLERR)
		cgiErrorOrHungUp(eventFd);
	if (client->_isCgi)
	{
		if (events & EPOLLIN)
			client->_io.readFromChildFd(client);
		if (events & EPOLLOUT && client->_request.begin()->_isWrite)
			client->_io.writeToChildFd(client);
	}
	return (true);
}

void	Epoll::cgiErrorOrHungUp(int cgiFd)
{
	std::cerr << "Error cgi fd: " << cgiFd << std::endl;
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
	Helper::setCloexec(_connSock);
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
	if (events & (EPOLLHUP | EPOLLRDHUP))
		return (clientHungUp(client));
	if (events & EPOLLERR)
		return (clientError(client));
	if (events & EPOLLIN)  // Check if the event is for reading
		client->_request.back().clientRequest(client);
		// should handle read events
	if (events & EPOLLOUT) // Check if the event is for writing
		clientResponse(client);
}

void Epoll::IOFiles()
{
	for (std::list<Client*>::iterator clientIt = _lstIoClients.begin(); clientIt != _lstIoClients.end();)
	{
		std::list<Client*>::iterator nextIt = clientIt;
		++nextIt;
		if ((*clientIt)->_request.begin()->_isRead)
			(*clientIt)->_io.readFromFile(*clientIt);
		else if ((*clientIt)->_request.begin()->_isWrite)
			(*clientIt)->_io.writeToFd(*clientIt);
		if ((*clientIt)->_io.getFd() == -1)
			_lstIoClients.erase(clientIt);  // Erase from the list
		clientIt = nextIt;  // Move to the next element
	}
}

void	Epoll::clientRetrievalError(int event_fd)
{
	std::cerr << "Error:\tretrieving client" << std::endl;
	removeClientEpoll(event_fd);
}

void	Epoll::clientHungUp(Client* client)
{
	// Handle error or hung up situation
	std::cerr << "Client hung up on fd: " << client->getFd() << std::endl;
	removeClient(client); // Remove the client from epoll and close the connection
}

void	Epoll::clientError(Client* client)
{
	std::cerr << "Client error up on fd: " << client->getFd() << std::endl;
	std::cerr << strerror(errno) << std::endl;
	removeClient(client); // Remove the client from epoll and close the connection
}

void	Epoll::clientResponse(Client* client)
{
	if ((client->_isCgi && client->_request.begin()->_response->getBodySize() > 0) || !client->_isCgi  || (client->_isCgi && client->_cgi.getCgiDone())) {
		client->_request.begin()->_response->sendResponse(client);
		// std::cout << "\n" << client->_request.begin()->_response->getIsFinished() << std::endl;
		// std::cout << client->_request.begin()->_response->getBodySize() << std::endl;
	}
	if (client->_request.begin()->_response->getIsFinished())
	{
		Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLIN);
		// client->_request.begin()->requestReset();
		if (client->_request.size() > 1)
			client->_request.pop_front();
		if (client->_request.size() > 1)
		{
			try {
				client->_request.begin()->executeMethod(client->getFd(), client);
			}
			catch (std::exception &e) {
				client->_request.begin()->_response->error(*client->_request.begin(), e.what(), client);
			}
		}
		client->_cgi = HandleCgi();
	}
}

void	Epoll::addCgiClientToEpollMap(int pipeFd, Client* client)
{
	//std::cout << "added pipeFd:\t" << pipeFd << " to Epoll map" << std::endl;
	_mpCgiClient.insert(std::make_pair(pipeFd, client));
}

void	Epoll::removeCgiClientFromEpoll(int pipeFd)
{
	std::map<int, Client*>::iterator it = _mpCgiClient.find(pipeFd);
	if (it != _mpCgiClient.end())
	{
		std::cout << "removeCgiClient from Epoll: " << pipeFd << std::endl;
		_mpCgiClient.erase(it);
		removeClientEpoll(pipeFd);
	}
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
	client->_cgi.closeCgi(client);
	removeClientEpoll(client->getFd());
	removeClientIo(client);
	removeClientFromServer(client->_server, client->getFd());
}

int	Epoll::getFd() const
{
	return (_epollFd);
}

void	Epoll::addClientIo(Client* client, std::string mode)
{
	_lstIoClients.push_back(client);
	if (mode == "read")
		client->_request.begin()->_isRead= true;
	else if (mode == "write")
		client->_request.begin()->_isWrite = true;
	else
		throw("500");
}

void	Epoll::removeClientIo(Client* client)
{
	_lstIoClients.remove(client);
	client->_request.begin()->_isRead = false;
	client->_request.begin()->_isWrite = false;
}
