#include "Epoll.hpp"
#include "Server.hpp"
#include "Response.hpp"
#include "main.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <sys/types.h>
#include <vector>
#include "Response.hpp"

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
	_ev.events = EPOLLIN;
	for (vSrv::iterator it = servers.begin(); it != servers.end(); ++it)
	{
		it->_epoll = this;
		// Retrieve the list of listening sockets from the server
		const lstSocs&	sockets = it->getLstnSockets();
		// Iterate over each listening socket
		for (lstSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
			registerSocket(it->getFdSocket());
	}
}

bool	Epoll::registerSocket(int fd)
{
	// Set the event flags to monitor for incoming connections
	_ev.events = EPOLLIN;  // Set event to listen for incoming data
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
			if (!NewClient(servers, _events[i].data.fd))  // Check if the event corresponds to one of the listening sockets
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
	socklen_t _addrlen = sockIt->getAddressLen();
	// Accept a new client connection on the listening socket
	_connSock = accept4(sockIt->getFdSocket(), (struct sockaddr *) &sockIt->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (_connSock < 0)
	{
		std::cerr << "Error:\taccept4 failed" << std::endl;
		return (false);  // Skip to the next socket if accept fails
	}
	std::cout << "New client connected: FD " << _connSock << std::endl;
	// Add the new client file descriptor to the server's list of connected clients
	if (!registerSocket(_connSock))
		return (false);
	Client	tmp(_connSock, sockIt->getPort(), &serv);
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
		clientRequest(client);
		// should handle read events
	if (events & EPOLLOUT) // Check if the event is for writing
		// should handle write events
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

int	Epoll::clientRequest(Client* client)
{

	// Read data from the client
	int		event_fd = client->getFd();
	bool	writeFlag = false;
	std::vector<char> buffer;  // Zero-initialize the buffer for incoming data
	ssize_t count = read(event_fd, &buffer[0], buffer.size());  // Read data from the client socket

	while (!client->_request.getReadingFinished())
	{
		try
		{
			buffer.resize(SOCKET_BUFFER_SIZE);
			ssize_t count = read(event_fd, &buffer[0], buffer.size());
			if (count == -1)
				return (invalidRequest(client));
			else if (count == 0)
				return (emptyRequest(client));
			else
				validRequest(client->_server, buffer, count, client->_request);
		}
		catch (std::exception &e)
		{
			if (static_cast<std::string>(e.what()) == TELNETSTOP) {
				Epoll::removeClient(client);
			} else {
				Response::FallbackError(event_fd, client->_request, static_cast<std::string>(e.what()));
			}
			std::cout << "exception: " << e.what() << std::endl;
			writeFlag = true;
			return OK;
		}
	}
	if (!writeFlag)
	{
		try {
			client->_request.executeMethod(event_fd, client);
		}
		catch (std::exception &e) {
			Response::FallbackError(event_fd, client->_request, static_cast<std::string>(e.what()));
		}

		std::cout << "-------------------------------------" << std::endl;
	}
	(void) count;
	writeFlag = false;
	std::map<std::string, std::string> testMap = client->_request.getHeaderMap();
	std::cout << client->_request.getMethodName() << " " << client->_request.getMethodPath() << " " << client->_request.getMethodProtocol() << std::endl;
	std::cout << "map size: " << testMap.size() << std::endl;
	client->_request.requestReset();
	return (0);
}

int	Epoll::invalidRequest(Client* client)
{
	if (errno == EINTR)
		return (-1);
	// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		removeClient(client);  // Close the socket on other read errors
	return (-1); // Move to the next event
}

int	Epoll::emptyRequest(Client* client)
{
	std::cout << "Client disconnected: FD " << client->getFd() << std::endl;
	removeClient(client);
	return (-1); // Move to the next event
}

void	Epoll::validRequest(Server* serv, std::vector<char> buffer, ssize_t count, Request& request)
{
	(void) serv;
	// std::cout << "test1: " << request.getFirstLineChecked() << std::endl;
	buffer.resize(count);
	// if (_buffer.size() == 5)
	// std::cout << (int) (unsigned char)_buffer[0] << " & " << (int) (unsigned char)_buffer[1] << " & " << (int) (unsigned char)_buffer[2] << " & " << (int) (unsigned char)_buffer[3] << " & " << (int) (unsigned char)_buffer[4] << " & " << _buffer.size() << std::endl;
	if(request.getFirstLineChecked()) {
		request.checkLine(buffer);
	} else {
		request.checkFirstLine(buffer);
	}
	// std::cout << "test2: " << request.getFirstLineChecked() << std::endl;
	//--- This should be here
	// std::string bufferRead(buffer.begin(), buffer.end());
	// size_t pos = bufferRead.find("cgi-bin");
	// if (pos != std::string::npos)
	// 	HandleCgi cgi(request.getMethodPath(), _connSock, serv);
}

void	Epoll::clientResponse(Client* client)
{
	(void)client;
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
	serv->removeClient(fd);
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
