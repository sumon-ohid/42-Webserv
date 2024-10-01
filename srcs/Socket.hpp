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
	int					_server_fd;
	int					_port;
	int					_addrlen;
	int					_new_socket;
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

	class	SocketBindingError;
	class	SocketListenError;
	class	SocketCreationError;
	class	SocketAcceptError;
};

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc);
