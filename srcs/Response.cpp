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

Response::Response() : _isChunk(false), _headerSent(false), _finishedSending(false), _closeConnection(false), _header(""), _body(""), _sessionId(""), _bytesSent(0) {}

Response::Response(const Response& other) : _isChunk(other._isChunk),
_headerSent(other._headerSent), _finishedSending(other._finishedSending), _closeConnection(other._closeConnection),
 _header(other._header), _body(other._body), _sessionId(other._sessionId),
 _bytesSent(other._bytesSent), _methodAndPath(other._methodAndPath) {}

Response& Response::operator=(const Response& other) {
	if (this == &other)
		return *this;

	_isChunk = other._isChunk;
	_headerSent = other._headerSent;
	_finishedSending = other._finishedSending;
	_closeConnection = other._closeConnection;
	_header = other._header;
	_body = other._body;
	_sessionId = other._sessionId;
	_bytesSent = other._bytesSent;
	_methodAndPath = other._methodAndPath;
	return *this;
}

Response::~Response() {}

Response*	Response::clone() const {
	return new Response(*this);
}

bool	Response::getIsChunk() {
	return _isChunk;
}

std::string Response::createHeaderString(Request& request, const std::string& body, std::string statusCode) {
	std::stringstream ss;
	std::string statusMessage = "";
	std::string url;
	bool	isRedirection = false;

	Helper::checkStatus(statusCode, statusMessage);

	if (statusCode[0] == '3') {
		isRedirection = true;
		url = body;
	}

	//-- BONUS : cookies
	_sessionId = request.getSessionId();
	if (_sessionId.empty())
		_sessionId = Helper::generateRandomId();

	if (request.hasMethod())
		ss << request.getMethodProtocol() << " " << statusCode << " " << statusMessage << "\r\n";
	else
	 	ss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
	ss << "Server: " << "webserv 1.0" << "\r\n";
	ss << "Date: " << Helper::getActualTimeStringGMT() << "\r\n";
	if (request.hasMethod() && !isRedirection)
		ss << "Content-Type: " << request.getMethodMimeType() << "\r\n";
	else if (!isRedirection)
	 	ss << "Content-Type: text/html\r\n";
	if (isRedirection)
		ss << "Location: " << body << "\r\n";
	if (this->getIsChunk())
		ss << "Transfer-Encoding: chunked\r\n";
	else if (!isRedirection)
		ss << "Content-Length: " << (body.size()) << "\r\n";
	else
		ss << "Content-Length: 0\r\n";
	if (_closeConnection || isRedirection)
		ss << "Connection: " << "close" << "\r\n";
	else
		ss << "Connection: " << "keep-alive" << "\r\n";
	if (!_sessionId.empty() && !isRedirection) //-- BONUS : cookies
        ss << "Set-Cookie: session=" << _sessionId << ";SameSite=Lax; Path=/; HttpOnly; Max-Age=3600;r\n";
	ss << "\r\n";

	return ss.str();
}

void Response::createHeaderAndBodyString(Request& request,std::string& body, std::string statusCode, Client* client) {
	if ( body.size() > CHUNK_SIZE)
		_isChunk = true;
	if (_header.empty())
		_header = createHeaderString(request, body, statusCode);
	_body += body;
	Helper::modifyEpollEventClient(*client->_epoll, client, EPOLLOUT);
}

void	Response::sendResponse(Client* client)
{
	_bytesSent = 0;
	if (_isChunk)
		prepareChunk(client);
	else
		sendSimpleResponse(client);
}

void	Response::prepareChunk(Client* client)
{
	if (!_headerSent)
	{
		_bytesSent = send(client->getFd(), _header.c_str(), _header.size(), 0);
		_headerSent = true;
	}
	else
	{
		if (!_body.empty())
			sendContentChunk(client);
		else
			sendNullChunk(client);
	}
	if (_bytesSent < 0)
	{
		std::cerr << "Error sending chunk response" << std::endl;
		client->_cgi.closeCgi(client);
		_finishedSending = true;
	}
}

void	Response::sendContentChunk(Client* client)
{
	std::ostringstream ss1;
	ss1 << std::hex << std::min(static_cast<unsigned long>(CHUNK_SIZE), _body.size()) << "\r\n";
	std::string header = ss1.str();
	std::string message = header + _body.substr(0, CHUNK_SIZE) + "\r\n";
	_bytesSent = send(client->getFd() , message.c_str(), message.size(), 0);
	_bytesSent -= header.size() + 2;
	if (_bytesSent > 0)
		_body.erase(0, _bytesSent);
}

void	Response::sendNullChunk(Client* client)
{
	_bytesSent = send(client->getFd() , "0\r\n\r\n", 5, 0);
	if (_bytesSent > 0)
	{
		_bytesSent -= 5;
		_finishedSending = true;
		std::cout << _methodAndPath << std::endl;
    	std::cout << BOLD GREEN << "Response sent to client successfully 🚀" << RESET << std::endl;
		std::cout << "--------------------------------------" << std::endl;
	}
}

void	Response::sendSimpleResponse(Client* client)
{
	_body = _header + _body + "\r\n";
	_bytesSent = send(client->getFd() , _body.c_str(), _body.size(), 0);
	if (_bytesSent < 0)
		throw std::runtime_error("Error writing to socket in Response::fallbackError!!");
	_finishedSending = true;
	if (client->_io.getFd() > -1)
		close(client->_io.getFd() );
	std::cout << _methodAndPath << std::endl;
    std::cout << BOLD GREEN << "Response sent to client successfully 🚀" << RESET << std::endl;
	std::cout << "--------------------------------------" << std::endl;
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
	std::cerr << BOLD RED << "Error: " + statusCode << ", method & path: " << _methodAndPath << RESET << std::endl;
}

void	Response::error(Request& request, std::string statusCode, Client *client)
{
	signal(SIGPIPE, SIG_IGN);

	std::string statusMessage = "";
    Helper::checkStatus(statusCode, statusMessage);

	std::map<std::string, std::string> errorPages;
	std::string errorPage;
	if (request._servConf)
		errorPage = request._servConf->getErrorPage();
	else
		errorPages = client->_server->getServerConfig().getErrorPages();

	if (errorPages.find(statusCode) != errorPages.end() || !errorPage.empty())
	{
		try
		{
			ErrorHandle errorHandle;
			errorHandle.prepareErrorPage(client, statusCode);
			if (request.hasMethod())
				request.setMethodMimeType("error.html");

			//-- this will create a new error file, error code will be the file name
			//-- this will modify the error page replacing the status code, message and page title
			//-- this will return the modified error page as a string
			std::string errorBody = errorHandle.modifyErrorPage();
			createHeaderAndBodyString(request, errorBody, statusCode, client);
			std::cerr << BOLD RED << "Error: " + statusCode << ", method & path: " << _methodAndPath << RESET << std::endl;
		}
		catch (std::exception &e)
		{
			try {
				std::cerr << BOLD RED << "ERROR: " << e.what() << " " << statusMessage << RESET << std::endl;
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
