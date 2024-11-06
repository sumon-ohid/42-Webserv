#include <exception>
#include <ios>
#include <ostream>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../includes/Response.hpp"
#include "../includes/ErrorHandle.hpp"
#include "../includes/Request.hpp"
#include "../includes/Helper.hpp"

Response::Response() : _socketFd(-1), _isChunk(false), _headerSent(false), _finishedSending(false), _closeConnection(false),
 _bytesSentOfBody(0), _header(""), _body(""), _mimeType(""), _bytesSent(0), _totalBytesSent(0) {}

Response::Response(const Response& other) : _socketFd(other._socketFd), _isChunk(other._isChunk),
_headerSent(other._headerSent), _finishedSending(other._finishedSending), _closeConnection(other._closeConnection),
 _bytesSentOfBody(other._bytesSentOfBody), _header(other._header), _body(other._body), _mimeType(other._mimeType),
 _bytesSent(other._bytesSent), _totalBytesSent(other._totalBytesSent) {}

Response& Response::operator=(const Response& other) {
	if (this == &other)
		return *this;

	_socketFd = other._socketFd;
	_isChunk = other._isChunk;
	_headerSent = other._headerSent;
	_finishedSending = other._finishedSending;
	_closeConnection = other._closeConnection;
	_bytesSentOfBody = other._bytesSentOfBody;
	_header = other._header;
	_body = other._body;
	_mimeType = other._mimeType;
	_bytesSent = other._bytesSent;
	_totalBytesSent = other._totalBytesSent;
	return *this;
}

Response::~Response() {}

Response*	Response::clone() const {
	return new Response(*this);
}

bool	Response::getIsChunk() {
	return _isChunk;
}

// to check if the statusCode exists - else respond with internal server error

std::string Response::createHeaderString(Request& request, const std::string& body, std::string statusCode) {
	std::stringstream ss;
	std::string statusMessage = "";

	Helper::checkStatus(statusCode, statusMessage);
	if (request.hasMethod())
		ss << request.getMethodProtocol() << " " << statusCode << " " << statusMessage << "\r\n";
	else
	 	ss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	ss << "Server: " << "webserv 1.0" << "\r\n";
	ss << "Date: " << Helper::getActualTimeStringGMT() << "\r\n";
	if (request.hasMethod())
		ss << "Content-Type: " << request.getMethodMimeType() << "\r\n";
	else
	 	ss << "Content-Type: text/html\r\n";
	if (this->getIsChunk())
		ss << "Transfer-Encoding: chunked\r\n";
	else
		ss << "Content-Length: " << (body.size() + 1) << "\r\n"; // BP: + 1 plus additional \r\n at the end?
	if (_closeConnection)
		ss << "Connection: " << "close" << "\r\n"; // BP: to check
	else
		ss << "Connection: " << "keep-alive" << "\r\n";
	ss << "\r\n";

	return ss.str();
}

// Connection: keep-alive
// Connection: Transfer-Encoding

void Response::createHeaderAndBodyString(Request& request, std::string& body, std::string statusCode, Client* client) {
	if ( body.size() > CHUNK_SIZE)
		_isChunk = true;
	if (_header.empty())
		_header = createHeaderString(request, body, statusCode);
	_body += body;
	Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLOUT);
}

void	Response::sendResponse(Client* client, int socketFd, Request& request) {
	long bytesSent = 0;
	if (_isChunk) {
		if (!_headerSent)
		{
			bytesSent = send(socketFd, _header.c_str(), _header.size(), 0);
			std::cout << "header sent; bytes sent:\t" << bytesSent << std::endl;
			_headerSent = true;
		}
		else
		{
			bytesSent = sendChunks(client, _body);
			_bytesSentOfBody += bytesSent;
			if (bytesSent > 0)
			{
				_body.erase(0, bytesSent);
				// if (_body.size() > 0)
				// 	Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLOUT);
				std::cout << "size of bytes that will be eliminated: " << bytesSent << std::endl;
			}
		}

		if (bytesSent < 0)
		{
			std::cerr << "Error sending chunk response" << std::endl; // BP: client closed? change
			Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLIN); // BP: check if it is protected
			return;
		}
	}
	else
	{
		std::string total = _header + _body +  "\r\n";
		bytesSent = send(socketFd , total.c_str(), total.size(), 0);
		if (bytesSent < 0)
			throw std::runtime_error("Error writing to socket in Response::fallbackError!!"); // BP: check where it is catched
		Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLIN);
		request.requestReset();
	}
}

long	Response::sendChunks(Client* client, std::string& chunkString) {
	std::ostringstream ss1;
	long bytesSent = 0;
	std::cout << "in sendChunks - chunkString size:\t" << chunkString.size() << std::endl;
	if (client->_isCgi)
		std::cout << "in sendChunks, client is cgi" << std::endl;
	if (client->_cgi.getCgiDone())
		std::cout << "in sendChunks, cgi has finished" << std::endl;
	if (!chunkString.empty())
	{
		ss1 << std::hex << std::min(static_cast<unsigned long>(CHUNK_SIZE), chunkString.size()) << "\r\n";
		std::string header = ss1.str();
		std::string message = header + chunkString.substr(0, CHUNK_SIZE) + "\r\n";
		bytesSent = send(client->getFd() , message.c_str(), message.size(), 0);
		std::cout << "bytes sent:\t" << bytesSent << std::endl;
		return (bytesSent - header.size() - 2);
	}
	else if (client->_isCgi && client->_cgi.getCgiDone())
	{
		std::cout << "client is cgi and cgi is done 0 byte" << std::endl;
		bytesSent = send(client->getFd() , "0\r\n\r\n", 5, 0);
		// Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLOUT);
		_finishedSending = true; // BP: is this necessary?
	}
	else if (!client->_isCgi)
	{
		std::cout << "normal 0 byte" << std::endl;
		bytesSent = send(client->getFd() , "0\r\n\r\n", 5, 0);
		_finishedSending = true; // BP: is this necessary?
	}
	return (0);
}

void	Response::fallbackError(Request& request, std::string statusCode, Client* client) {
	_closeConnection = true;

	if (request.hasMethod())
		request.setMethodMimeType("fallback.html");

	std::string statusMessage = "";
	std::stringstream ss;
	Helper::checkStatus(statusCode, statusMessage);

	ss << "<!DOCTYPE html>\r\n<html>\r\n";
	ss << "<head><title>" << statusCode << " " << statusMessage << "</title></head>\r\n";
	ss << "<body>\r\n<center><h1>" << statusCode << " - " << statusMessage << "</h1></center>\r\n";
	ss << "<hr><center>" << "webserv 1.0" << "</center>\r\n";
	ss << "</body>\r\n</html>\r\n";

	std::string body = ss.str();
	createHeaderAndBodyString(request, body, statusCode, client);
	std::cerr << BOLD RED << "Error: " + statusCode << ", method: " << request.getMethodName() << ", path: " << request.getMethodPath() << RESET << std::endl;
}

//hardcoding of internal server Error (check case stringsteam fails)
// what if connection was closed due to no connection, whatever?
// check what happens with a fd if the connection is closed
// what happens in case of a write error?
// try again mechanism?
// how to decide implement if we keep a connection open or closing?

//-- SUMON: Trying to make it work with ErrorHandle class
// what if we have a write error?
void	Response::error(Request& request, std::string statusCode, Client *client)
{
	signal(SIGPIPE, SIG_IGN);

	std::map<std::string, std::string> errorPages;
	if (request._servConf)
		errorPages = request._servConf->getErrorPages();
	else
		errorPages = client->_server->getServerConfig().getErrorPages();
	if (errorPages.find(statusCode) != errorPages.end() || errorPages.empty())
	{
		try
		{
			ErrorHandle errorHandle;
			errorHandle.prepareErrorPage(client, statusCode);
			if (request.hasMethod())
				request.setMethodMimeType(errorHandle.getNewErrorFile()); // BP: no mime type here, maybe just set it to HTML?
			// std::cout << "mime-type: "<< request.getMethodMimeType() << std::endl;
			// std::cout << "file: "<< errorHandle.getNewErrorFile() << std::endl;
			//-- this will create a new error file, error code will be the file name
			//-- this will modify the error page replacing the status code, message and page title
			//-- this will return the modified error page as a string
			std::string errorBody = errorHandle.modifyErrorPage();
			createHeaderAndBodyString(request, errorBody, statusCode, client);

			// std::string total = _header + _body;
			// bytesSent = send(socketFd , total.c_str(), total.size(), 0);
			// if (bytesSent == -1)
			// 	throw std::runtime_error("Error writing to socket in Response::error!!");
			// else
			// 	std::cerr << BOLD RED << "Error: " + statusCode << ", method: " << request.getMethodName() << ", path: " << request.getMethodPath() << RESET << std::endl;
			// NOTE: sometimes write fails, subject says errno is forbidden for read and write
			// SUB : You must never do a read or a write operation without going through poll() (or equivalent).
		}
		catch (std::exception &e)
		{
			try {
				std::cerr << BOLD RED << e.what() << RESET << std::endl;
				fallbackError(request, statusCode, client);
			} catch (std::exception& e) {
				std::cerr << BOLD RED << e.what() << RESET << std::endl;
			}
		}
	}
	else
	{
		try {
			fallbackError(request, statusCode, client);
		}
		catch (std::exception &e) {
			std::cerr << BOLD RED << e.what() << RESET << std::endl;
		}
	}

}

void	Response::setIsChunk(bool setBool)
{
	_isChunk = setBool;
}

void	Response::addToBody(const std::string& str)
{
	_body += str;
}

size_t	Response::getBodySize() const
{
	return (_body.size());
}

bool	Response::getIsFinished()
{
	return (_finishedSending);
}
