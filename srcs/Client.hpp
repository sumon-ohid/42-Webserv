#pragma once

#include <ctime>
#include "Request.hpp"

class Server;

class Client
{
private:
	int				_fd;
	int				_port;
	time_t			_lastActive;
	unsigned		_numRequests;

public:
	Server*			_server;
	Request			_request; // BP: can there be one or multiple requests at the same time
	
	// Coplien's form
	Client();
	Client(int, int, Server*);
	~Client();
	Client(const Client&);
	Client&	operator=(const Client&);
	bool	operator==(const Client& other) const;

	// setters
	// sets the fd of the client to the value passed
	void		setFD(int);
	// sets the port of the client to the value passed
	void		setPort(int);
	// sets lastActove of the client to the value passed
	void		setLastActive(time_t);
	// will add 1 to numRequests of the client
	void		numRequestAdd1();

	// getters
	// returns the fd of the client
	int			getFd() const;
	// returns the port of the client
	int			getPort() const;
	// returns the time when the client was last active
	time_t		getLastActive() const;
	// returns the num of requests the client has already made
	unsigned	getNumRequest() const;
};
