#include "../includes/IO.hpp"
#include "../includes/Client.hpp"
#include "../includes/Response.hpp"
#include "../includes/Helper.hpp"
#include <cstddef>
#include <cstring>
#include <features.h>
#include <sys/types.h>
#include <cerrno>

IO::IO() : _fd(-1), _size(0), _byteTracker(0), _totalBytesSent(0), _mimeCheckDone(false) {}
IO::IO(const IO& orig) : _byteTracker(orig._byteTracker), _totalBytesSent(orig._totalBytesSent),
_response(orig._response), _responseStr(orig._responseStr),
_mimeCheckDone(orig._mimeCheckDone)
{}
IO&	IO::operator=(const IO& rhs)
{
	if (this != &rhs)
	{
		_byteTracker = rhs._byteTracker;
		_totalBytesSent = rhs._totalBytesSent;
		_response = rhs._response;
		_responseStr = rhs._responseStr;
		_mimeCheckDone = rhs._mimeCheckDone;
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
	// std::cerr << strerror(errno) << std::endl;
	client->_epoll->removeCgiClientFromEpoll(_fd);
	throw std::runtime_error("500");
}

void	IO::finishWrite(Client* client)
{
	if (client->_isCgi && client->_request.begin()->_isWrite == true)
		finishWriteCgi(client); // pass _pipeOut[0]
	resetIO(client);
}

void	IO::finishWriteCgi(Client* client)
{
	if (client->_request.begin()->_isRead == false)
		Helper::addFdToEpoll(client, client->_cgi.getPipeOut(0), EPOLLIN); // pass _pipeOut[0]
	client->_request.begin()->_isWrite = false;
	client->_request.begin()->_isRead = true;
	client->_request.begin()->_response->setIsChunk(true);
}

void	IO::resetIO(Client* client)
{
	(void) client; // BP: remove client
	close (_fd);
	_fd = -1;
	_size = 0;
	_byteTracker = 0;
	_totalBytesSent = 0;
	_response.clear();
	_responseStr.clear();
	_mimeCheckDone = false;
}

void	IO::readFromChildFd(Client* client)
{
	client->_cgi.checkWaitPid();
	_fd = client->_cgi.getPipeOut(0);
	readFromFd();
	checkReadOrWriteError(client);
	if (_byteTracker == 0)
		finishReadingFromFd(client);
	if (!_mimeCheckDone)
		MimeTypeCheck(client);
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
		finishReadingFromFd(client);
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
	if (client->_isCgi)
	{
		if (!_mimeCheckDone)
			MimeTypeCheck(client);
		client->_cgi.setCgiDone(true);
		client->_epoll->removeCgiClientFromEpoll(_fd);
	}
	resetIO(client);
}

void	IO::MimeTypeCheck(Client* client)
{
	//*** This is to handle mime types for cgi scripts
	_responseStr = std::string(_response.data(), _byteTracker);
	size_t pos = _responseStr.find("Content-Type:");
	std::string setMime;
	extractMimeType(pos, setMime);
	// std::cout << (client->_request.begin()->hasMethod() ? "method there" : "no method") << std::endl;
	client->_request.begin()->setMethodMimeType(setMime);
	_mimeCheckDone = true;
	size_t bodyStart = _responseStr.find("\r\n\r\n");
	if (bodyStart != std::string::npos)
		_responseStr.erase(0, bodyStart += 5);
}

void	IO::extractMimeType(size_t pos, std::string& setMime)
{
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
    	setMime = ".html";
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
