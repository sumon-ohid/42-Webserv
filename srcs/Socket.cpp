#include "../includes/Socket.hpp"
#include "../includes/ServerConfig.hpp"

#include <cerrno>
#include <netinet/in.h>
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <utility>
#include <sstream>

// Coplien
Socket::Socket() : _fd(-1), _port(-1) {}
Socket::Socket(int port) : _fd(-1), _port(port)
{}
Socket::~Socket(){}
Socket::Socket(const Socket &orig) : _fd(orig._fd), _port(orig._port), _addrlen(orig._addrlen), _address(orig._address), _configs(orig._configs)
{}
Socket&	Socket::operator=(const Socket &rhs)
{
	if (this != &rhs)
	{
		_fd = rhs._fd;
		_port = rhs._port;
		_addrlen = rhs._addrlen;
		_address = rhs._address;
		_configs = rhs._configs;
	}
	return (*this);
}

bool	operator==(const sockaddr_in& lhs, const sockaddr_in& rhs)
{
	return (lhs.sin_family == rhs.sin_family &&
            lhs.sin_port == rhs.sin_port &&
            std::memcmp(&lhs.sin_addr, &rhs.sin_addr, sizeof(lhs.sin_addr)) == 0);
}

bool	Socket::operator==(const Socket& other) const
{
	return (_fd == other._fd &&
			_port == other._port &&
			_addrlen == other._addrlen &&
			_address == other._address) &&
			_configs == other._configs;
}

bool 	Socket::operator<(const Socket& other) const 
{
	if (_fd != other._fd)
		return _fd < other._fd;
	if (_port != other._port)
		return _port < other._port;
	if (_addrlen != other._addrlen)
		return _addrlen < other._addrlen;
	if (_address.sin_addr.s_addr != other._address.sin_addr.s_addr)
		return _address.sin_addr.s_addr < other._address.sin_addr.s_addr;
	return _address.sin_port < other._address.sin_port;
}

// Functions

void	Socket::setUpSocket(const std::string& hostname, ServerConfig& servConf, ServerManager& sm)
{
	socketSetUpAddress(hostname, servConf, sm);
}

void	Socket::createSocket(struct addrinfo* p)
{
    // Creates a file descriptor (fd) for a socket, specifying the address family (IPv4),
    // the socket type (SOCK_STREAM for TCP), and setting the socket to non-blocking mode
    // (SOCK_NONBLOCK) while using the default protocol (0).
	_fd = socket(p->ai_family, p->ai_socktype | SOCK_NONBLOCK | SOCK_CLOEXEC, p->ai_protocol);
    // if there was an error creating the socket
	if (_fd < 0)
		throw std::runtime_error("socket - could not create socket");
}


void	Socket::bindToSocketAndListen(struct addrinfo* p)
{
	int	opt = 1;
		/// Enable SO_REUSEADDR (set opt = 1) to allow binding to a port in TIME_WAIT state,
		// facilitating quick restarts of the server without waiting for the socket to be released.
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
			throw std::runtime_error("socket - could not set SO_REUSEADDR option");
    // binds the socket file descriptor to the specified port and IP address
	if (bind(_fd, p->ai_addr, p->ai_addrlen) < 0)
		throw std::runtime_error("socket - binding to socket failed");
	// Instructs the socket to listen for incoming connection requests,
    // allowing up to SOCKET_MAX_LISTEN connections to be queued.
    if (listen(_fd, SOCKET_MAX_LISTEN) < 0)
		throw std::runtime_error("socket - listen failed");
}

void	Socket::addConfig(const std::string& hostIp, std::vector<LocationConfig> locConf)
{
	_configs.insert(std::make_pair(hostIp, locConf));
}

void	Socket::socketSetUpAddress(const std::string& hostname, ServerConfig& servConf, ServerManager& sm)
{
    struct addrinfo hints, *res;

	// Prepare the hints structure
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;	  // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP

	std::ostringstream portStream;
	portStream << _port; // Convert the integer to string
	std::string portStr = portStream.str();

	// Resolve the hostname
	int status = getaddrinfo(hostname.c_str(), portStr.c_str(), &hints, &res);
	if (status != 0)
		throw std::runtime_error(gai_strerror(status));
	createSocketForAddress(hostname, res, servConf, sm);
	// Create a listening socket
}

void	Socket::createSocketForAddress(const std::string& hostname, struct addrinfo* res, ServerConfig& servConf, ServerManager& sm)
{
	for (struct addrinfo* p = res; p != NULL; p = p->ai_next) 
	{
		struct sockaddr_in* addr = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
		std::string ipHost(reinterpret_cast<const char*>(&addr->sin_addr), sizeof(addr->sin_addr));
	// Create the socket with SOCK_NONBLOCK flag
		if (sm.IpPortCombinationNonExistent(hostname, ipHost, *this, servConf)) //otherwise add to existing one inside IpPort... the Location File with this Hostname
		{
			createSocket(p);
			bindToSocketAndListen(p);
			sm.addNewSocketIpCombination(_port, ipHost);
			_configs.insert(std::make_pair(hostname, servConf.getLocations()));
		}
	}
	freeaddrinfo(res);
}

int	Socket::getFdSocket() const
{
	return (_fd);
}

int	Socket::getPort() const
{
	return (_port);
}

socklen_t	Socket::getAddressLen(void) const
{
	return (_addrlen);
}

sockaddr_in	Socket::getAddress(void) const
{
	return (_address);
}

sockaddr_in&	Socket::getAddress()
{
	return (_address);
}

// ostream

std::ostream&	operator<<(std::ostream &os, const std::vector<char> &vc)
{
	for (std::vector<char>::const_iterator it = vc.begin(); it != vc.end(); ++it)
		os << *it;
	os << std::endl;
	return (os);
}

std::vector<LocationConfig>*	Socket::getConfig(std::string& hostname)
{
	mHstLoc::iterator it = _configs.find(hostname);
	if (it != _configs.end())
		return (&it->second);
	else
		return (NULL);
}