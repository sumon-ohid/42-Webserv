#pragma once

#include <climits>
#include <netinet/in.h>
#include <ostream>
#include <sys/socket.h>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <iostream>

#define	SOCKET_BUFFER_SIZE		30000
#define SOCKET_MAX_LISTEN		1000

class Socket
{
private:
	int					_fd;
	int					_port;
	socklen_t			_addrlen;
	sockaddr_in			_address;
public:
	Socket();
	Socket(int);
	~Socket();
	Socket(const Socket&);
	Socket&	operator=(const Socket&);

	// sets up a socket to use at a specified port
	void	setUpSocket(void);
	/**
	* Creates a socket file descriptor for TCP communication.
	*
	* Initializes a socket with:
	* - Address family: IPv4 (AF_INET)
	* - Socket type: TCP (SOCK_STREAM)
	* - Mode: Non-blocking (SOCK_NONBLOCK)
	* - Protocol: Default (0)
	*/
	void	createSocket(void);
	/**
 	* Binds the socket to a specific port and IP address.
 	*
 	* Instructs the socket to listen for incoming connection requests.
 	*/
	void	bindToSocketAndListen(void);
	/**
 	* Sets up the socket address for binding.
 	*
 	* This function:
 	* - Initializes the address structure for IPv4.
 	* - Binds to all interfaces (INADDR_ANY).
 	* - Sets the port number.
 	* - Clears the padding field.
 	* - Logs the port number.
 	*/
	void	socketSetUpAddress(void);
	void	socketLoop(void);

	const int&			getFdSocket(void) const;
	int&				getFdSocket(void);
	const int&			getPort(void) const;
	socklen_t&			getAddressLen(void);
	const socklen_t& 	getAddressLen() const;
	const sockaddr_in&	getAddress(void) const;
};

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc);
