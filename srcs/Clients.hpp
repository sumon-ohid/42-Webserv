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
	Clients();
	~Clients();
	Clients(const Clients&);
	Clients&	operator=(const Clients&);

	void addClient(int fd);
	void removeClient(int fd);
	void listClients();
	bool isClientConnected(int fd);

	const lstInt&	getClientFds() const;
	size_t			getClientCount() const;
};