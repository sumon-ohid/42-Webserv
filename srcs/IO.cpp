#include "../includes/IO.hpp"
#include "../includes/Client.hpp"
#include "../includes/Response.hpp"
#include "../includes/Helper.hpp"
#include <cstddef>
#include <cstring>
#include <features.h>
#include <stdexcept>
#include <sys/types.h>

IO::IO() : _fd(-1), _size(0), _byteTracker(0), _totalBytesSent(0), _mimeCheckDone(false), _timeout(false) {}

IO::IO(const IO& orig) : _byteTracker(orig._byteTracker), _totalBytesSent(orig._totalBytesSent),
	_response(orig._response), _responseStr(orig._responseStr),
	_mimeCheckDone(orig._mimeCheckDone), _timeout(orig._timeout) {}

IO&	IO::operator=(const IO& rhs)
{
	if (this != &rhs)
	{
		_byteTracker = rhs._byteTracker;
		_totalBytesSent = rhs._totalBytesSent;
		_response = rhs._response;
		_responseStr = rhs._responseStr;
		_mimeCheckDone = rhs._mimeCheckDone;
		_timeout = rhs._timeout;
	}
	return (*this);
}

IO::~IO() {}


void	IO::writeToChildFd(Client* client)
{
	_fd = client->_cgi.getPipeIn(1);
	size_t bytesToWrite = std::min(static_cast<std::size_t>(IO_SIZE), client->_request.begin()->_requestBody.size() - _totalBytesSent);
	_response = std::vector<char>(client->_request.begin()->_requestBody.begin() + _totalBytesSent, client->_request.begin()->_requestBody.begin() + _totalBytesSent + bytesToWrite );
	_size =  client->_request.begin()->_requestBody.size();
	writeToFd(client);
}

void	IO::writeToFd(Client* client)
{
	size_t	bytesToWrite = std::min(_response.size(), static_cast<std::size_t>(IO_SIZE));
	_byteTracker = write(_fd, _response.data(), bytesToWrite);
	checkReadOrWriteError(client);
	_response.erase(_response.begin(), _response.begin() + _byteTracker);
	if ((_totalBytesSent += _byteTracker) >= _size)
		finishWrite(client);
}

void	IO::checkReadOrWriteError(Client* client)
{
	if (_byteTracker > -1)
		return;
	if (!client->_isCgi)
	{
		client->_epoll->removeCgiClientFromEpoll(_fd);
		client->_io.resetIO();
		throw std::runtime_error("500");
	}
}

void	IO::finishWrite(Client* client)
{
	if (client->_isCgi && client->_request.begin()->_isWrite == true)
		finishWriteCgi(client);
	resetIO();
}

void	IO::finishWriteCgi(Client* client)
{
	if (client->_request.begin()->_isRead == false)
		Helper::addFdToEpoll(client, client->_cgi.getPipeOut(0), EPOLLIN);
	client->_epoll->removeCgiClientFromEpoll(client->_cgi.getPipeIn(1));
	_fd = -1;
	client->_request.begin()->_isWrite = false;
	client->_request.begin()->_isRead = true;
	client->_request.begin()->_response->setIsChunk(true);
}

void	IO::resetIO()
{
	if (_fd > 0)
		close (_fd);
	_fd = -1;
	_size = 0;
	_byteTracker = 0;
	_totalBytesSent = 0;
	_response.clear();
	_responseStr.clear();
	_mimeCheckDone = false;
	_timeout = false;
}
void	IO::readFromChildFd(Client* client)
{
	if (_timeout || client->_cgi.getPipeOut(0) < 0)
		return;
	client->_cgi.checkWaitPid(client);
	if (_timeout)
		return;
	_fd = client->_cgi.getPipeOut(0);
	readFromFd();
	checkReadOrWriteError(client);
	if (_byteTracker == -1)
		return;
	if (_byteTracker == 0)
		return (finishReadingFromFd(client));
	if (!_mimeCheckDone) {
		MimeTypeCheck(client);
	}
	else
		_responseStr = std::string(_response.data(), _byteTracker);
	client->_request.begin()->_response->createHeaderAndBodyString(*client->_request.begin(), _responseStr, "200", client);
	_byteTracker = 0;
}

void	IO::readFromFile(Client* client)
{
	readFromFd();
	checkReadOrWriteError(client);
	if (_byteTracker == 0)
		return (finishReadingFromFd(client));
	_responseStr = std::string(_response.data(), _byteTracker);
	client->_request.begin()->_response->createHeaderAndBodyString(*client->_request.begin(), _responseStr, "200", client);
	_byteTracker = 0;
}

void	IO::readFromFd()
{
	_response.resize(IO_SIZE, '\0');
	if (_fd > 0)
		_byteTracker = read(_fd, _response.data(), _response.size());
	_totalBytesSent += _byteTracker;
}

void	IO::finishReadingFromFd(Client* client)
{
	std::cout << "finished reading" << std::endl;
	if (client->_isCgi)
	{
		if (!_mimeCheckDone)
			MimeTypeCheck(client);
		client->_isCgi = false;
		client->_epoll->removeCgiClientFromEpoll(_fd);
	}
	client->_cgi.setCgiDone(true);
	resetIO();
}

void	IO::MimeTypeCheck(Client* client)
{
	//*** This is to handle mime types for cgi scripts
	_responseStr = std::string(_response.data(), _byteTracker);
	size_t pos = _responseStr.find("Content-Type:");
	std::string setMime;
	extractMimeType(pos, setMime, client);
	client->_request.begin()->setMethodMimeType(setMime);
	_mimeCheckDone = true;
	size_t bodyStart = _responseStr.find("\r\n\r\n");
	if (bodyStart != std::string::npos)
		_responseStr.erase(0, bodyStart += 5);
}

void	IO::extractMimeType(size_t pos, std::string& setMime, Client* client)
{
	(void) client;
	if (pos != std::string::npos)
	{
		std::string mimeType = _responseStr.substr(pos + 14, _responseStr.find("\r\n", pos) - pos - 14);

		std::map<std::string, std::string> mimeTypes = Helper::mimeTypes;
		std::map<std::string, std::string>::iterator it;
		for (it = mimeTypes.begin(); it != mimeTypes.end(); it++)
		{
			if (mimeType == it->second)
			{
				setMime = it->first;
				break;
			}
		}
	}
	if (setMime.empty())
    {
		client->_isCgi = false;
		client->_cgi.setCgiDone(true);
		client->_epoll->removeCgiClientFromEpoll(_fd);
		resetIO();
		throw std::runtime_error("415");
	}
}

size_t	IO::getSize() const
{
	return (_size);
}

int	IO::getFd() const
{
	return (_fd);
}

void	IO::setFd(int fd)
{
	_fd = fd;
}

void	IO::setSize(size_t size)
{
	_size = size;
}

void	IO::setTimeout(bool val)
{
	_timeout = val;
}

bool	IO::getTimeout() const
{
	return (_timeout);
}