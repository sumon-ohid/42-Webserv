#pragma once

#include <climits>
#include <netinet/in.h>
#include <ostream>
#include <sys/socket.h>
#include <vector>
#include <cstring>
#include <exception>
#include <unistd.h>
#include <iostream>

#define	SOCKET_BUFFER_SIZE		30000
#define SOCKET_MAX_LISTEN		1000
#define SOCKET_BINDING_ERROR	"socket - binding to socket failed"
#define	SOCKET_LISTEN_ERROR		"socket - listen failed"
#define	SOCKET_CREATION_ERROR	"socket - could not create socket"
#define	SOCKET_ACCEPT_ERROR		"socket - accept failed"

class Socket
{
private:
	int					_fd;
	int					_port;
	socklen_t			_addrlen;
	int					_newSocket;
	long				_valread;
	sockaddr_in			_address;
	std::vector<char>	_buffer;
public:
	Socket();
	Socket(int);
	~Socket();
	Socket(const Socket&);
	Socket&	operator=(const Socket&);

	void	createSocket(void);
	void	bindToSocket(void);
	void	socketSetUpAddress(void);
	void	socketLoop(void);

	const int&			getFdSocket(void) const;
	const int&			getPort(void) const;
	socklen_t&			getAddressLen(void);
	socklen_t 			getAddressLen() const;
	const sockaddr_in&	getAddress(void) const;


	class	SocketBindingError;
	class	SocketListenError;
	class	SocketCreationError;
	class	SocketAcceptError;
};

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc);
