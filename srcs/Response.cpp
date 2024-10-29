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

Response::Response() : _socketFd(-1), _isChunk(false), _bytesSent(0), _message(""), _mimeType("") {}

Response::Response(const Response& other) : _socketFd(other._socketFd), _isChunk(other._isChunk), _bytesSent(other._bytesSent), _message(other._message), _mimeType(other._mimeType) {
	(void) other;
}

Response& Response::operator=(const Response& other) {
	if (this == &other)
		return *this;

	this->_socketFd = other._socketFd;
	this->_isChunk = other._isChunk;
	this->_bytesSent = other._bytesSent;
	this->_message = other._message;
	this->_mimeType = other._mimeType;
	return *this;
}

Response::~Response() {}

Response*	Response::clone() const {
	return new Response(*this);
}

static std::string createHeaderString(Request& request, std::string& body, std::string statusCode) {
	std::stringstream ss;
	std::map<std::string, std::string>::const_iterator it;
	std::string statusMessage;

	it = Helper::statusCodes.find(statusCode);
	if (it == Helper::statusCodes.end()) {
		statusCode = "500";
		statusMessage = "Internal Server Error";
	} else {
		statusMessage = it->second;
	}

	if (request.hasMethod())
		ss << request.getMethodProtocol() << " " << statusCode << " " << statusMessage << "\n";
	else
	 	ss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\n";
	ss << "Server: " << "webserv 1.0" << "\n";
	ss << "Date: " << Helper::getActualTimeStringGMT() << "\n";
	if (request.hasMethod())
		ss << "Content-Type: " << request.getMethodMimeType() << "\n";
	else
	 	ss << "Content-Type: text/html\n";
	ss << "Content-Length: " << (body.size() + 1) << "\n"; // + 1 plus additional \n at the end?
	ss << "Connection: " << "connectionClosedOrNot" << "\n";

	return ss.str();
}

static std::string createHeaderAndBodyString(Request& request, std::string& body, std::string statusCode) {
	std::stringstream ss;
	ss << createHeaderString(request, body, statusCode) << "\n";
	ss << body << "\n"; //BP: \n at the end - do we need this?

	return ss.str();
}

void	Response::header(int socketFd, Request& request, std::string& body) {
	std::string headString = createHeaderString(request, body, "200");
	write(socketFd , headString.c_str(), headString.size());
}

void	Response::headerAndBody(int socketFd, Request& request, std::string& body) {
	if (body.size() > CHUNK_SIZE) {
		sendWithChunkEncoding(socketFd, request, body);
	} else {
		std::string totalString = createHeaderAndBodyString(request, body, "200");
		// std::cout << totalString << std::endl;
		write(socketFd , totalString.c_str(), totalString.size());
	}
}

void	Response::fallbackError(int socketFd, Request& request, std::string statusCode) {

	std::stringstream ss;
	std::map<std::string, std::string>::const_iterator it;
	it = Helper::statusCodes.find(statusCode);
	std::string statusMessage;
	if (it == Helper::statusCodes.end()) {
		statusCode = "500";
		statusMessage = "Internal Server Error";
	} else {
		statusMessage = it->second;
	}

	ss << "<!DOCTYPE html>\n<html>\n<head><title>" << statusCode << " " << statusMessage << "</title></head>\n";
	ss << "<body>\n<center><h1>" << statusCode << " " << statusMessage << "</h1></center>\n";
	ss << "<hr><center>" << "webserv 1.0" << "</center>\n</body>\n</html>\n";
	std::string body = ss.str();
	std::string totalString = createHeaderAndBodyString(request, body, statusCode);
	ssize_t writeReturn = write(socketFd, totalString.c_str(), totalString.size());
	if (writeReturn == -1)
		throw std::runtime_error("Error writing to socket in Response::fallbackError!!");
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

	std::map<std::string, std::string> errorPages;
	if (request._servConf)
		errorPages = request._servConf->getErrorPages();
	else
		errorPages = client->_server->getServerConfig().getErrorPages();
	if (errorPages.find(statusCode) != errorPages.end() || errorPages.empty())
	{
		ssize_t writeReturn = 0;
		try
		{
			ErrorHandle errorHandle;
			errorHandle.prepareErrorPage(client, statusCode);
			if (request.hasMethod())
				request.setMethodMimeType(errorHandle.getNewErrorFile());
			//-- this will create a new error file, error code will be the file name
			//-- this will modify the error page replacing the status code, message and page title
			//-- this will return the modified error page as a string
			std::string errorBody = errorHandle.modifyErrorPage();
			std::string totalString = createHeaderAndBodyString(request, errorBody, statusCode);
			writeReturn = write(socketFd, totalString.c_str(), totalString.size());
			if (writeReturn == -1)
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

void	Response::sendChunks(int socketFd, std::string chunkString) {
	std::ostringstream ss;
	ss << std::hex << chunkString.size() << "\r\n";
	unsigned long int hexSize = ss.str().size();
	ss << chunkString << "\r\n";
	ssize_t bytesSent = send(socketFd, ss.str().c_str(), ss.str().size(), 0);
	// check if -1 or 0 => error
	bytesSent -= hexSize;
	// move message
	// if (message.empty())
		//send(socketFd, "0\r\n\r\n", 5, 0);

	usleep(100000); // change to

}

void	Response::sendWithChunkEncoding(int socketFd, Request& request, std::string& body) {
	(void) request;
	std::string ChunkStartHeader = "HTTP/1.1 200 OK\r\n"
                          "Content-Type: video/mp4\r\n"
                          "Transfer-Encoding: chunked\r\n"
                          "\r\n";
	//ChunkStartHeaderSent false? then:
	send(socketFd, ChunkStartHeader.c_str(), ChunkStartHeader.size(), 0);


	for (unsigned long i = 0; i < body.size(); i += CHUNK_SIZE) { // add bytesSent
		sendChunks(socketFd, body.substr(i, CHUNK_SIZE));
		// check whats better chunk loop or always go back to epoll loop
	}
	send(socketFd, "0\r\n\r\n", 5, 0);
}
