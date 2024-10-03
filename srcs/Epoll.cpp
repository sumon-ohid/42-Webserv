#include "Epoll.hpp"
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

void	Epoll::registerLstnSockets(const Server& serv)
{
	// Set the event flags to monitor for incoming connections
	_ev.events = EPOLLIN;
	// Retrieve the list of listening sockets from the server
	vecSocs	sockets = serv.getLstnSockets();
    // Iterate over each listening socket
	for (vecSocs::iterator it = sockets.begin(); it != sockets.end(); ++it)
	{
		// Update the event data with the file descriptor of the current socket
		_ev.data.fd = it->getFdSocket();
		// Add the socket to the epoll instance for monitoring
        if (epoll_ctl(_fd, EPOLL_CTL_ADD, it->getFdSocket(), &_ev) == -1)
			std::cerr << "Error:\tepoll_ctl: listen_sock" << std::endl;
	}
}

void Epoll::EpollMonitoring(const Server &serv)
{
	// Infinite loop to process events
	while (1)
	{
		// Wait for events on the epoll instance
		_nfds = epoll_wait(_fd, _events, MAX_EVENTS, -1);
		if (_nfds == -1)
			throw std::runtime_error("epoll_wait failed");
		const vecSocs& sockets = serv.getLstnSockets();  // Use reference to avoid copying
		// Iterate over the events returned by epoll_wait
		for (int i = 0; i < _nfds; ++i)
		{
			int event_fd = _events[i].data.fd;  // File descriptor for this event

			// Check if the event corresponds to one of the listening sockets
			bool is_listening_socket = false;
			for (vecSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
			{
				// Compare event_fd with the listening socket's file descriptor
				if (event_fd == it->getFdSocket())
				{
					// This is a listening socket, accept new client connection
					socklen_t _addrlen = it->getAddressLen();
					_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
					if (_connSock < 0)
					{
						std::cerr << "Error:\taccept4 failed" << std::endl;
						continue;  // Skip to the next socket if accept fails
					}
					std::cout << "New client connected: FD " << _connSock << std::endl;

					// Register the new client socket with EPOLLIN (ready to read)
					_ev.events = EPOLLIN;  // Set event to listen for incoming data
					_ev.data.fd = _connSock;  // Set the file descriptor for the client socket
					if (epoll_ctl(_fd, EPOLL_CTL_ADD, _connSock, &_ev) == -1)
					{
						// If epoll_ctl fails, close the new socket
						close(_connSock);
					}
					is_listening_socket = true;  // Mark that we've handled a listening socket
					break;  // Exit the loop as we have accepted a new client
				}
			}

			// If it's a new client connection, skip to the next event
			if (is_listening_socket)
				continue;

			// Handle events on existing client connections
			if (_events[i].events & EPOLLIN)  // Check if the event is for reading
			{
				// Read data from the client
				char buffer[300000] = {0};  // Zero-initialize the buffer for incoming data
				ssize_t count = read(event_fd, buffer, sizeof(buffer));  // Read data from the client socket

				if (count == -1)
				{
					// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
					if (errno != EAGAIN && errno != EWOULDBLOCK)
					{
						close(event_fd);  // Close the socket on other read errors
					}
					continue;  // Move to the next event
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

					// Prepare a simple HTTP response
					std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!\n";
					ssize_t sent = write(event_fd, response.c_str(), response.size());  // Send response to the client

					if (sent == -1)
					{
						// Handle write failure by closing the socket
						close(event_fd);
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

void Epoll::EpollEventMonitoring(const Server& serv)
{
	for (int i = 0; i < _nfds; ++i)
	{
		int event_fd = _events[i].data.fd;  // File descriptor for this event
		// Check if the event corresponds to one of the listening sockets
		bool newClient = EpollNewClient(serv, event_fd);
		// If it's a new client connection, skip to the next event
		if (newClient)
			continue;
		// Handle events on existing client connections
		if (_events[i].events & EPOLLIN)  // Check if the event is for reading
			EpollExistingClient(event_fd);
	}
}

bool	Epoll::EpollNewClient(const Server &serv, const int &event_fd)
{
	// retrieve listening sockets
	const vecSocs& sockets = serv.getLstnSockets();
	// iterate over listening sockets
	for (vecSocs::const_iterator it = sockets.begin(); it != sockets.end(); ++it)
	{
		// Compare event_fd with the listening socket's file descriptor
		if (event_fd == it->getFdSocket())
		{
			// This is a listening socket, accept new client connection
			socklen_t _addrlen = it->getAddressLen();
			_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (_connSock < 0)
			{
				std::cerr << "Error:\taccept4 failed" << std::endl;
				continue;  // Skip to the next socket if accept fails
			}
			std::cout << "New client connected: FD " << _connSock << std::endl;

			
			// Register the new client socket with EPOLLIN (ready to read)
			_ev.events = EPOLLIN;  // Set event to listen for incoming data
			_ev.data.fd = _connSock;  // Set the file descriptor for the client socket
			if (epoll_ctl(_fd, EPOLL_CTL_ADD, _connSock, &_ev) == -1)
				close(_connSock);
			return (true);  // Mark that we've handled a listening socket
		}
	}
	return (false);
}

bool	Epoll::EpollAcceptNewClient(const Server &serv, const vecSocs::const_iterator& it)
{
	socklen_t _addrlen = it->getAddressLen();
	_connSock = accept4(it->getFdSocket(), (struct sockaddr *) &it->getAddress(), &_addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
	if (_connSock < 0)
	{
		std::cerr << "Error:\taccept4 failed" << std::endl;
		return (false);  // Skip to the next socket if accept fails
	}
	std::cout << "New client connected: FD " << _connSock << std::endl;
	serv.
	vecInt&	clntSocks = serv.getCnctSockets();
	clntSocks.push_back(_connSock);
	return (true);
}

int	Epoll::EpollExistingClient(const int &event_fd)
{
	// Read data from the client
	char buffer[300000] = {0};  // Zero-initialize the buffer for incoming data
	ssize_t count = read(event_fd, buffer, sizeof(buffer));  // Read data from the client socket

	if (count == -1)
	{
		// Handle read error (ignore EAGAIN and EWOULDBLOCK errors)
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			close(event_fd);  // Close the socket on other read errors
		return (-1); // Move to the next event
	}
	else if (count == 0)
	{
		// Client disconnected, close the socket and remove from epoll
		std::cout << "Client disconnected: FD " << event_fd << std::endl;
		close(event_fd);
		return (-1); // Move to the next event
	}
	else
	{
		// Successfully read data, print it and respond
		std::cout << "Received " << count << " bytes: " << buffer << std::endl;
		// Prepare a simple HTTP response
		std::string response = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!\n";
		ssize_t sent = write(event_fd, response.c_str(), response.size());  // Send response to the client

		if (sent == -1)
			// Handle write failure by closing the socket
			close(event_fd);
		else
			std::cout << "Response sent to client on FD " << event_fd << std::endl;
	}
	return (0);
}

void	Epoll::EpollRoutine(const Server& serv)
{
	createEpoll();
	registerLstnSockets(serv);
	EpollMonitoring(serv);
}
