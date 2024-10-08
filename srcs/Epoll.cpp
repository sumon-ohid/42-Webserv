#include "Epoll.hpp"
#include "HandleCgi.hpp"
#include "Server.hpp"
#include "Response.hpp"
#include "main.hpp"
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <sys/types.h>
#include <vector>

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

void	Epoll::createEpoll()
{
	_epollFd = epoll_create1(EPOLL_CLOEXEC);
	if (_epollFd == -1)
		std::runtime_error("epoll - creation failed");
}

void	Epoll::registerLstnSockets(const Server& serv)
{

	_ev.events = EPOLLIN;
	// Retrieve the list of listening sockets from the server
	const lstSocs&	sockets = serv.getLstnSockets();
    // Iterate over each listening socket
	for (lstSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
		registerSocket(it->getFdSocket());
}

void	Epoll::registerSocket(int fd)
{
	// Set the event flags to monitor for incoming connections
	_ev.events = EPOLLIN;  // Set event to listen for incoming data
	_ev.data.fd = fd;  // Set the file descriptor for the client socket
	// Add the socket to the epoll instance for monitoring
	if (epoll_ctl(_epollFd, EPOLL_CTL_ADD, fd, &_ev) == -1)
		close(fd);
}

void Epoll::EpollMonitoring(Server& serv)
{
	// Infinite loop to process events
	while (1)
	{
		// Wait for events on the epoll instance
		_nfds = epoll_wait(_epollFd, _events, MAX_EVENTS, -1);
		if (_nfds == -1)
		{
			if (errno == EINTR)
				break;
			else
				throw std::runtime_error("epoll_wait failed");
		}
		for (int i = 0; i < _nfds; ++i)
		{
			int event_fd = _events[i].data.fd;  // File descriptor for this event
			// Check if the event corresponds to one of the listening sockets
			bool newClient = EpollNewClient(serv, event_fd);
			// Handle events on existing client connections
			if (!newClient && _events[i].events & EPOLLIN)  // Check if the event is for reading
				EpollExistingClient(serv, event_fd);
		}
	}
}


bool	Epoll::EpollNewClient(Server &serv, const int &event_fd) // possible change: implement a flag that server does not accept new connections anymore to be able to shut it down
{
	// retrieve listening sockets
	const lstSocs& sockets = serv.getLstnSockets();
	// iterate over listening sockets
	for (lstSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
	{
		// Compare event_fd with the listening socket's file descriptor
		if (event_fd == it->getFdSocket())
		{
			// This is a listening socket, accept new client connection
			if (!EpollAcceptNewClient(serv, it))
				continue;
			registerSocket(_connSock);
			return (true);  // Mark that we've handled a listening socket
		}
	}
	return (false);
}

bool	Epoll::EpollAcceptNewClient(Server &serv, const lstSocs::const_iterator& it)
{
	// Get the length of the address associated with the current listening socket
	socklen_t _addrlen = it->getAddressLen();
	// Accept a new client connection on the listening socket
	_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (_connSock < 0)
	{
		std::cerr << "Error:\taccept4 failed" << std::endl;
		return (false);  // Skip to the next socket if accept fails
	}

	//--- Handle cgi
	// std::string bufferRead(_buffer.begin(), _buffer.end());
	// size_t pos = bufferRead.find("cgi-bin");
	// if (pos != std::string::npos)
	// 	HandleCgi cgi(bufferRead, _connSock, serv);

	//-- End of cgi

	std::cout << "New client connected: FD " << _connSock << std::endl;
	// Add the new client file descriptor to the server's list of connected clients
	serv.addClientFd(_connSock);
	std::cout << "in EpollAcceptNewClient" << std::endl;
	return (true);
}

int	Epoll::EpollExistingClient(Server& serv, const int &event_fd)
{
	// Read data from the client
	bool	writeFlag = false;
	Request request; // add to client?
	std::vector<char> buffer;  // Zero-initialize the buffer for incoming data
	ssize_t count = read(event_fd, &buffer[0], buffer.size());  // Read data from the client socket

	while (!request.getReadingFinished())
	{
		try
		{
			buffer.resize(SOCKET_BUFFER_SIZE);
			ssize_t count = read(event_fd, &buffer[0], buffer.size());
			if (count == -1)
				return (invalidRequest(serv, event_fd));
			else if (count == 0)
				return (emptyRequest(serv, event_fd));
			else
				validRequest(serv, buffer, count, request);
		}
		catch (std::exception &e)
		{
			if (static_cast<std::string>(e.what()) == TELNETSTOP) {
				Epoll::removeFd(serv, event_fd);
			} else {
				Response::FallbackError(event_fd, request, static_cast<std::string>(e.what()));
			}
			std::cout << "exception: " << e.what() << std::endl;
			writeFlag = true;
			break;
		}
	}
	_buffer = buffer;
	if (!writeFlag)
	{
		request.executeMethod(event_fd);
		// std::string test = "this is a test";
		// Response::headerAndBody(event_fd, request, test);
		// write(_new_socket , hello.c_str() , hello.size());
    	std::cout << "------------------Hello message sent-------------------" << std::endl;
	}
	(void) count;
	writeFlag = false;
	std::map<std::string, std::string> testMap = request.getHeaderMap();
	std::cout << request.getMethodName() << " " << request.getMethodPath() << " " << request.getMethodProtocol() << std::endl;
	std::cout << "map size: " << testMap.size() << std::endl;
	request.requestReset();
	return (0);
}

int	Epoll::invalidRequest(Server& serv, const int &event_fd)
{
	if (errno == EINTR)
		return (-1);
	// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
	if (errno != EAGAIN && errno != EWOULDBLOCK)
		removeFd(serv, event_fd);  // Close the socket on other read errors
	return (-1); // Move to the next event
}

int	Epoll::emptyRequest(Server& serv, const int &event_fd)
{
	std::cout << "Client disconnected: FD " << event_fd << std::endl;
	removeFd(serv, event_fd);
	return (-1); // Move to the next event
}

void	Epoll::validRequest(Server& serv, std::vector<char> buffer, ssize_t count, Request& request)
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
	std::string bufferRead(buffer.begin(), buffer.end());
	size_t pos = bufferRead.find("cgi-bin");
	if (pos != std::string::npos)
		HandleCgi cgi(request.getMethodPath(), _connSock, serv);
}

void	Epoll::EpollRoutine(Server& serv)
{
	createEpoll();
	registerLstnSockets(serv);
	EpollMonitoring(serv);
}

void	Epoll:: removeFdEpoll(int fd)
{
	if (epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL) == -1)
        std::cerr << "Error removing FD from epoll: " << strerror(errno) << std::endl;
    // Close the client socket
    close(fd);
}

void	Epoll::removeFdClients(Server& serv, int fd)
{
	serv.removeClientFd(fd);
}

void	Epoll::removeFd(Server& serv, int fd)
{
	removeFdEpoll(fd);
	removeFdClients(serv, fd);
}

int	Epoll::getFd() const
{
	return (_epollFd);
}
