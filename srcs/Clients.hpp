#pragma once

#include <cstddef>
#include <list>
#include <iostream>
#include <algorithm>

typedef std::list<int>	lstInt;

class Clients
{
private:
	lstInt	_clnts;
public:
	// Coplien's form

	Clients();
	~Clients();
	Clients(const Clients&);
	Clients&	operator=(const Clients&);

	// client fd handling

	// adds clients' to the internal list, keeping track of their fds
	void addClient(int fd);
	//  removes clients' fds from the internal list
	void removeClient(int fd);
	// prints the fds of the currently connected clients
	void listClients() const;
	// returns true if client is connected and false if not
	bool isClientConnected(int fd) const;

	// Getters

	// returns a list containing the clients' fds
	const lstInt&	getClientFds() const;
	// returns the amount of clients currently connected
	size_t			getClientCount() const;
};