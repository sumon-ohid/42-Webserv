#include "Clients.hpp"

// ------------- Coplien's Form -------------

Clients::Clients() {}
Clients::~Clients() {}
Clients::Clients(const Clients& orig) : _clnts(orig._clnts) {}
Clients&	Clients::operator=(const Clients& rhs)
{
	if (this != &rhs)
		_clnts = rhs._clnts;
	return (*this);
}

// ------------- Client Management -------------

void Clients::addClient(int fd) 
{
    _clnts.push_back(fd);
}

void Clients::removeClient(int fd) 
{
	_clnts.remove(fd); // Removes all occurrences of fd
}

void Clients::listClients() const
{
	for (lstInt::const_iterator it = _clnts.begin(); it != _clnts.end(); ++it) 
		std::cout << "FD: " << *it << std::endl;
}

bool Clients::isClientConnected(int fd) const
{
	return (std::find(_clnts.begin(), _clnts.end(), fd) != _clnts.end());
}

// ------------- getters -------------

const lstInt&	Clients::getClientFds() const
{
	return (_clnts);
}

size_t			Clients::getClientCount() const
{
	return (_clnts.size());
}