#include <exception>
#include <ios>
#include <ostream>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "../includes/Response.hpp"
#include "../includes/ErrorHandle.hpp"
#include "../includes/Request.hpp"
#include "../includes/Helper.hpp"

Response::Response() : _socketFd(-1), _isChunk(false), _headerSent(false), _finishedSending(false), _closeConnection(false), _bytesSentOfBody(0), _header(""), _message(""), _mimeType("") {}

Response::Response(const Response& other) : _socketFd(other._socketFd), _isChunk(other._isChunk),  _headerSent(other._headerSent), _finishedSending(other._finishedSending), _closeConnection(other._closeConnection), _bytesSentOfBody(other._bytesSentOfBody), _header(other._header), _message(other._message), _mimeType(other._mimeType) {}

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
	_message = other._message;
	_mimeType = other._mimeType;
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


void Response::createHeaderAndBodyString(Request& request, std::string& body, std::string statusCode) {
	std::stringstream ss;
	_header = createHeaderString(request, body, statusCode);
	_message = body + "\r\n";
}

void	Response::header(int socketFd, Request& request, std::string& body) { // BP check if used
	std::string headString = createHeaderString(request, body, "200");
	send(socketFd , headString.c_str(), headString.size(), 0);
}

void	Response::headerAndBody(int socketFd, Request& request, std::string& body) { // BP: change name to sendHeaderAndBody?
	if (_isChunk || body.size() > CHUNK_SIZE) {
		sendWithChunkEncoding(socketFd, request, body);
	} else {
		createHeaderAndBodyString(request, body, "200");
		// std::cout << totalString << std::endl;
		std::string total = _header + _message;
		send(socketFd , total.c_str(), total.size(), 0);
		// send(socketFd , _message.c_str(), _message.size(), 0);
	}
}

void	Response::fallbackError(int socketFd, Request& request, std::string statusCode) {
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
	createHeaderAndBodyString(request, body, statusCode);
	std::string total = _header + _message;
	ssize_t bytesSent = send(socketFd , total.c_str(), total.size(), 0);
	if (bytesSent == -1)
		throw std::runtime_error("Error writing to socket in Response::fallbackError!!"); // BP: check where it is catched
	else
		std::cerr << BOLD RED << "Error: " + statusCode << RESET << std::endl;
}

//hardcoding of internal server Error (check case stringsteam fails)
// what if connection was closed due to no connection, whatever?
// check what happens with a fd if the connection is closed
// what happens in case of a write error?
// try again mechanism?
// how to decide implement if we keep a connection open or closing?

//-- SUMON: Trying to make it work with ErrorHandle class
// what if we have a write error?
void	Response::error(int socketFd, Request& request, std::string statusCode, Client *client)
{
	signal(SIGPIPE, SIG_IGN);

	std::map<std::string, std::string> errorPages = client->_server->_serverConfig.getErrorPages();
	if (errorPages.find(statusCode) != errorPages.end() || errorPages.empty())
	{
		ssize_t bytesSent = 0;
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
			createHeaderAndBodyString(request, errorBody, statusCode);

			std::string total = _header + _message;
			bytesSent = send(socketFd , total.c_str(), total.size(), 0);
			if (bytesSent == -1)
				throw std::runtime_error("Error writing to socket in Response::error!!");
			else
				std::cerr << BOLD RED << "Error: " + statusCode << RESET << std::endl;
			// NOTE: sometimes write fails, subject says errno is forbidden for read and write
			// SUB : You must never do a read or a write operation without going through poll() (or equivalent).
		}
		catch (std::exception &e)
		{
			try {
				std::cerr << BOLD RED << e.what() << RESET << std::endl;
				fallbackError(socketFd, request, statusCode);
			} catch (std::exception& e) {
				std::cerr << BOLD RED << e.what() << RESET << std::endl;
			}
		}
	}
	else
	{
		try {
			fallbackError(socketFd, request, statusCode);
		}
		catch (std::exception &e) {
			std::cerr << BOLD RED << e.what() << RESET << std::endl;
		}
	}

}

#include <unistd.h>

long	Response::sendChunks(int socketFd, std::string chunkString) {
	if (chunkString.size() == 0) {
		// send(socketFd, "0\r\n\r\n", 5, 0); // BP: add this ev. when changing to epoll version
		return 0;
	}
	usleep(100000); // change to epoll
	std::ostringstream ss1;
	ss1 << std::hex << chunkString.size() << "\r\n";
	// unsigned long int hexSize = ss.str().size();
	long bytesSent = send(socketFd, ss1.str().c_str(), ss1.str().size(), 0);
	if (bytesSent < 0)
		return bytesSent;
	std::ostringstream ss2;
	ss2 << chunkString << "\r\n";
	bytesSent = send(socketFd, ss2.str().c_str(), ss2.str().size(), 0);

	return bytesSent - 2;
}

void	Response::sendWithChunkEncoding(int socketFd, Request& request, std::string& body) {
	_isChunk = true;
	long bytesSent;

	std::string chunkStartHeader = createHeaderString(request, body, "200") + "\r\n"; // BP: createHeader ev. without body - and
	bytesSent = send(socketFd, chunkStartHeader.c_str(), chunkStartHeader.size(), 0);
	if (bytesSent < 0) {
		std::cerr << "Error sending response header" << std::endl;
		return;
	}

	for (unsigned long i = 0; i < body.size(); i += CHUNK_SIZE) { // add bytesSent
		bytesSent = sendChunks(socketFd, body.substr(i, CHUNK_SIZE));
		_bytesSentOfBody += bytesSent;
		std::cout << "bytes sent: " << _bytesSentOfBody<< std::endl;
		if (bytesSent < 0) {
			std::cerr << "Connection closed by client" << std::endl; // BP: ev. other message?
			break;
		}
		// change to implement: go back to epoll loop instead of for-loop
	}
	if (bytesSent >= 0)
		send(socketFd, "0\r\n\r\n", 5, 0); // for ending chunk encoding

	(void) request;
}

// EPolloutFlag setzen und am Ende Epol in setzen
