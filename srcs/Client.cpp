#include "../includes/Client.hpp"

// ------------- Coplien's Form -------------
Client::Client() : _fd(-1), _port(-1), _lastActive(0), _numRequests(0), _epoll(NULL), _server(NULL), _socket(NULL), _isCgi(false) {
	_request.push_back(Request());
};

Client::Client(int fd, int port, Server *serv, Socket* socket, Epoll* epoll) : _fd(fd), _port(port), _lastActive(0), _numRequests(0), _epoll(epoll), _server(serv), _socket(socket), _isCgi(false) {
	_request.push_back(Request());
}

Client::Client(const Client& orig) : _fd(orig._fd), _port(orig._port), _lastActive(orig._lastActive), _numRequests(orig._numRequests), _epoll(orig._epoll), _server(orig._server), _socket(orig._socket), _request(orig._request), _isCgi(orig._isCgi) {}

Client::~Client() {}

Client&	Client::operator=(const Client& rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_port = rhs._port;
		_lastActive = rhs._lastActive;
		_numRequests = rhs._numRequests;
		_epoll = rhs._epoll;
		_server = rhs._server;
		_socket = rhs._socket;
		_request = rhs._request;
		_isCgi = rhs._isCgi;
	}
	return (*this);
}
bool	Client::operator==(const Client& other) const
{
	return (_fd == other._fd &&
			_port == other._port &&
			_lastActive == other._lastActive &&
			_numRequests == other._lastActive &&
			_epoll == other._epoll &&
			_server == other._server &&
			_socket == other._socket &&
			_request == other._request &&
			_isCgi == other._isCgi);
}


void		Client::setFD(int fd)
{
	_fd = fd;
}

void		Client::setPort(int port)
{
	_port = port;
}

void		Client::setLastActive()
{
	_lastActive = time(NULL);
}

void		Client::numRequestAdd1()
{
	_numRequests++;
}

// getters

int			Client::getFd() const
{
	return (_fd);
}

int			Client::getPort() const
{
	return (_port);
}

time_t		Client::getLastActive() const
{
	return (_lastActive);
}

unsigned	Client::getNumRequest() const
{
	return (_numRequests);
}
