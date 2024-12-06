#include "../includes/Epoll.hpp"
#include "../includes/Server.hpp"
#include "../includes/Response.hpp"
#include "../includes/Helper.hpp"
#include "../includes/main.hpp"

#include <exception>
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

void	Epoll::routine(std::vector<Server> &servers)
{
	createEpoll();
	registerLstnSockets(servers);
	monitoring(servers);
}

void	Epoll::createEpoll()
{
	_epollFd = epoll_create(MAX_EVENTS);
	if (_epollFd == -1)
		std::runtime_error("epoll - creation failed");
	Helper::setCloexec(_epollFd);
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

void Epoll::monitoring(vSrv& servers)
{
	// Infinite loop to process events
	while (1)
	{
		// Wait for events on the epoll instance
		if (checkEpollWait(epoll_wait(_epollFd, _events, MAX_EVENTS, EPOLL_TIMEOUT_MS)) == -1 || stopSignal)
			break;
		if (!_nfds)
			continue;
		for (int i = 0; i < _nfds; ++i)
			if (!existingClient(_events[i].data.fd, _events[i].events))  // Check if the event corresponds to one of the listening sockets
				newClient(servers, _events[i].data.fd);
		ioFiles();
		checkTimeouts();
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
	if (_nfds == 0)
		checkTimeouts();
	return (_nfds);
}

bool	Epoll::existingClient(int eventFd, uint32_t events)
{
	std::map<int, Client*>::iterator it = _mpClients.find(eventFd);
	if (it == _mpClients.end())
		return (false);
	Client* client = it->second;
	if (client->_isCgi)
		handleCgiClient(client, eventFd, events);
	else
		handleRegularClient(client, events);
	return (true);
}

void	Epoll::handleCgiClient(Client* client, int eventFd, uint32_t events)
{
	try
	{
		if (events & (EPOLLHUP | EPOLLRDHUP))
		{
			if (client->_isCgi && !client->_cgi.getCgiDone())
				client->_io.readFromChildFd(client);
			else
			{
				endCgi(client);
				removeClientEpoll(eventFd);
			}
		}
		else if (events & EPOLLERR)
			cgiErrorOrHungUp(eventFd);
		else if (client->_request.begin()->_isRead)
		{
			client->_io.readFromChildFd(client);
			handleRegularClient(client, events);
		}
		else if (client->_request.begin()->_isWrite)
			client->_io.writeToChildFd(client);
	}
	catch (std::exception &e)
	{
		endCgi(client);
		client->_request.begin()->_response->clearBody();
		client->_cgi.setCgiDone(true);
		client->_request.begin()->_response->error(*client->_request.begin(), e.what(), client);
		client->_request.begin()->_response->addToBody("0\r\n\r\n");
	}
}

void	Epoll::endCgi(Client* client)
{
	client->_cgi.closeCgi(client);
	client->_isCgi = false;
	client->_io.resetIO(client);
}

void	Epoll::handleRegularClient(Client* client, uint32_t events)
{
	try
	{
		if (events & (EPOLLHUP | EPOLLRDHUP))
			return (clientHungUp(client));
		else if (events & EPOLLERR)
			return (clientError(client));
		if ((events & (EPOLLIN | EPOLLOUT)) && !client->_isCgi)
			client->setLastActive();
		if (events & EPOLLIN)  // Check if the event is for reading
			client->_request.back().clientRequest(client);
		else if (events & EPOLLOUT) // Check if the event is for writing
			clientResponse(client);
	}
	catch (std::exception &e)
	{
		if (!client->_request.begin()->_response->getIsFinished())
			client->_request.begin()->_response->error(*client->_request.begin(), e.what(),client);
	}
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

bool	Epoll::newClient(vSrv &servers, int event_fd)
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
				return (acceptNewClient(*servIt, sockIt));
	}
	return (false);
}

bool	Epoll::acceptNewClient(Server &serv, lstSocs::iterator& sockIt)
{
	// Get the length of the address associated with the current listening socket
	socklen_t _addrlen = sizeof(sockIt->getAddress());
	// Accept a new client connection on the listening socket
	_connSock = accept(sockIt->getFdSocket(), (struct sockaddr *) &sockIt->getAddress(), &_addrlen);
	if (_connSock < 0)
	{
		std::cerr << gai_strerror(_connSock);
		throw std::runtime_error("Error:\taccept failed");
		  // Skip to the next socket if accept fails
	}
	Helper::setCloexec(_connSock);
	Helper::setFdFlags(_connSock, O_NONBLOCK);
	if (DEBUG_MODE)
		std::cout << "New client connected: FD " << _connSock << std::endl;
	// Add the new client file descriptor to the server's list of connected clients
	if (!registerSocket(_connSock, EPOLLIN | EPOLLET))
		return (false);
	Helper::setCloexec(_connSock);
	Client	tmp(_connSock, sockIt->getPort(), &serv, &(*sockIt), this);
	tmp.setLastActive();
	serv.addClient(tmp);
	_mpClients.insert(std::make_pair(_connSock, serv.getClient(_connSock)));
	return (true);
}

void Epoll::ioFiles()
{
	for (std::list<Client*>::iterator clientIt = _lstIoClients.begin(); clientIt != _lstIoClients.end();)
	{
		std::list<Client*>::iterator nextIt = clientIt;
		++nextIt;
		try {
			if ((*clientIt)->_request.begin()->_isRead)
			{
				(*clientIt)->_io.readFromFile(*clientIt);
				// (*clientIt)->setLastActive();
			}
			else if ((*clientIt)->_request.begin()->_isWrite)
			{
				(*clientIt)->_io.writeToFd(*clientIt);
				// (*clientIt)->setLastActive();
			}
			if ((*clientIt)->_io.getFd() == -1)
				_lstIoClients.erase(clientIt);  // Erase from the list
		}
		catch (std::exception &e)
		{
			std::cout << BOLD RED << "ERROR: " << e.what() << RESET << std::endl;
			removeClient(*clientIt);
		}
		clientIt = nextIt;  // Move to the next element
	}
}

void	Epoll::checkTimeouts()
{
	for (std::map<int, Client*>::iterator it = _mpClients.begin(); it != _mpClients.end();)
	{
		std::map<int, Client*>::iterator nextIt = it;
		while (nextIt->second == it->second && nextIt != _mpClients.end())
			++nextIt;
		if ((it->second)->_isCgi)
		{
			try
			{
				(it->second)->_cgi.checkCgiTimeout(it->second);
			}
			catch (std::exception &e)
			{
				(it->second)->_request.begin()->_response->clearHeader();
				(it->second)->_request.begin()->_response->setIsChunk(false);
				(it->second)->_request.begin()->_response->error(*(it->second)->_request.begin(), e.what(), (it->second));
				removeCgiClientFromEpoll(it->second->_io.getFd());
			}
		}
		else
		{
			if (it != _mpClients.end() && it->second != NULL)
			{
				if ((it->second)->_server->getServerConfig().getTimeout() != -1 && Helper::getElapsedTime(it->second) > (it->second)->_server->getServerConfig().getTimeout())
				{
					if (it->first == it->second->getFd())
					{
						removeClient(it->second);
					}
					else
						removeCgiClientFromEpoll(it->first);
				}
			}
		}
		it = nextIt;
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
	if (DEBUG_MODE)
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
	if ((client->_isCgi && client->_request.begin()->_response->getBodySize() > 0) || !client->_isCgi  || (client->_isCgi && client->_cgi.getCgiDone()))
		client->_request.begin()->_response->sendResponse(client);
	if (client->_request.begin()->_response->getIsFinished())
	{
		if (client->_request.begin()->_response->getBodySize() == 0)
			clientRequestDone(client);
		else
			removeClient(client);
	}
}

void	Epoll::clientRequestDone(Client *client)
{
	Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLIN);
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

void	Epoll::addCgiClientToEpollMap(int pipeFd, Client* client)
{
	_mpClients.insert(std::make_pair(pipeFd, client));
}

void	Epoll::removeCgiClientFromEpoll(int pipeFd)
{
	std::map<int, Client*>::iterator it = _mpClients.find(pipeFd);
	if (it != _mpClients.end())
	{
		if (is_fd_valid(pipeFd))
		{
			_mpClients.erase(it);
			removeClientEpoll(pipeFd);
		}
	}
}

bool Epoll::is_fd_valid(int fd)
{
	return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

void	Epoll:: removeClientEpoll(int fd)
{
	if (DEBUG_MODE)
		std::cout << "remove fd: " << fd << std::endl;
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
		std::cerr << "Error removing fd " << fd << " from epoll: " << strerror(errno) << std::endl;
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
	if (client->_isCgi && !client->_cgi.getChildReaped())
		kill(client->_cgi.getPid(), SIGKILL);
	removeCgiClientFromEpoll(client->getFd());
	if (client->_io.getFd() > 0)
		removeCgiClientFromEpoll(client->_io.getFd());
	client->_cgi.closeCgi(client);
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
		throw std::runtime_error("500");
}

void	Epoll::removeClientIo(Client* client)
{
	_lstIoClients.remove(client);
	client->_request.begin()->_isRead = false;
	client->_request.begin()->_isWrite = false;
}
