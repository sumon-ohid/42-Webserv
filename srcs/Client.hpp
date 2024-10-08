#pragma once

#include <cstddef>
#include <ctime>
#include <list>
#include <iostream>
#include <algorithm>
#include "Request.hpp"

class Client
{
private:
	int			_fd;
	Request		_request;
	time_t		_last_active;
	unsigned	_numRequests;
	int			port;
public:
	// Coplien's form

	Client();
	Client(int);
	~Client();
	Client(const Client&);
	Client&	operator=(const Client&);

	// setters
	void		setFD(int);
	void		setLastActive(time_t);
	void		numRequestAdd1();

	// getters
	int			getFd();
	time_t		getLastActive();
	unsigned	getNumRequest();
	std::string	getMethodName();
	std::string	getMethodPath();
	std::string	getMethodProtocol();
	Request&	getRequest();


	// client fd handling

	// adds clients' to the internal list, keeping track of their fds
	// void addClient(int &fd);
	// //  removes clients' fds from the internal list
	// void removeClient(int fd);
	// // prints the fds of the currently connected clients
	// void listClients() const;
	// // returns true if client is connected and false if not
	// bool isClientConnected(int fd) const;

	// // Getters

	// // returns a list containing the clients' fds
	// const lstInt&	getClientFds() const;

	// lstInt&	getClientFds() ;
	// // returns the amount of clients currently connected
	// size_t			getClientCount() const;
};
