#include <ios>
#include <ostream>
#include <sstream>
#include <sys/socket.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "Response.hpp"
#include "ErrorHandle.hpp"

static std::map<std::string, std::string> initMap() {
	std::map<std::string, std::string> codes;
	codes["200"] = "OK";
	codes["201"] = "Created";
	codes["204"] = "No Content";
	codes["301"] = "Moved Permanently";
	codes["302"] = "Found";
	codes["400"] = "Bad Request";
	codes["401"] = "Unauthorized";
	codes["403"] = "Forbidden";
	codes["404"] = "Not Found";
	codes["405"] = "Not Allowed";
	codes["413"] = "Request Entity Too Large";
	codes["500"] = "Internal Server Error";
	codes["503"] = "Service Unavailable";
	codes["505"] = "HTTP Version Not Supported";
	return codes;
}

const std::map<std::string, std::string> Response::statusCodes = initMap();

Response::Response() {}

Response::Response(const Response& other) {
	(void) other;
}

Response& Response::operator=(const Response& other) {
	(void) other;
	return *this;
}

Response::~Response() {}

std::string Response::getActualTimeString() {
	std::string weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	std::stringstream ss;
	std::time_t now = std::time(0);
    std::tm* localNow = std::gmtime(&now);
	std::string weekday = weekdays[localNow->tm_wday];
	std::string month = months[localNow->tm_mon];

	ss << weekday << ", ";
	ss << std::setfill('0') << std::setw(2) << localNow->tm_mday << " " << month << " ";
	ss << (localNow->tm_year + 1900) << " ";
	ss << std::setfill('0') << std::setw(2) << localNow->tm_hour << ":";
	ss << std::setfill('0') << std::setw(2) << localNow->tm_min << ":";
	ss << std::setfill('0') << std::setw(2) << localNow->tm_sec << " GMT";
	return ss.str();
}

static std::string createHeaderString(Request& request, std::string& body, std::string statusCode) {
	std::stringstream ss;
	std::string statusMessage = Response::statusCodes.find(statusCode)->second;

	ss << request.getMethodProtocol() << " " << statusCode << " " << statusMessage << "\n"; // or use string directly
	ss << "Server: " << "someName" << "\n";
	ss << "Date: " << Response::getActualTimeString() << "\n";
	ss << "Content-Type: " << request.getMethodMimeType() << "\n";
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

void	Response::FallbackError(int socketFd, Request& request, std::string statusCode) {

	std::stringstream ss;
	std::string statusMessage = Response::statusCodes.find(statusCode)->second;
	request.setMethodMimeType("test.html");

	ss << "<!DOCTYPE html>\n<html>\n<head><title>" << statusCode << " " << statusMessage << "</title></head>\n";
	ss << "<body>\n<center><h1>" << statusCode << " " << statusMessage << "</h1></center>\n";
	ss << "<hr><center>" << "WEBSERV OR SERVERNAME?" << "</center>\n</body>\n</html>\n";
	std::string body = ss.str();
	std::string totalString = createHeaderAndBodyString(request, body, statusCode);
	write(socketFd, totalString.c_str(), totalString.size());
}

//hardcoding of internal server Error (check case stringsteam fails)
// what if connection was closed due to no connection, whatever?
// check what happens with a fd if the connection is closed
// what happens in case of a write error?
// try again mechanism?
// how to decide implement if we keep a connection open or closing?

//-- SUMON: Trying to make it work with ErrorHandle class
// what if we have a write error?
void	Response::FallbackError(int socketFd, Request& request, std::string statusCode, Client *client)
{
	signal(SIGPIPE, SIG_IGN);
	ssize_t writeReturn = 0;
	try
	{
		ErrorHandle errorHandle;
		errorHandle.prepareErrorPage(client, statusCode);
		request.setMethodMimeType(errorHandle.getNewErrorFile());

		//-- this will create a new error file, error code will be the file name
		//-- this will modify the error page replacing the status code, message and page title
		//-- this will return the modified error page as a string
		std::string errorBody = errorHandle.modifyErrorPage();
		std::string totalString = createHeaderAndBodyString(request, errorBody, statusCode);
		writeReturn = write(socketFd, totalString.c_str(), totalString.size());
		if (writeReturn == -1)
			throw std::runtime_error("Error writing to socket in Response::FallbackError!!");
		// NOTE: sometimes write fails, subject says errno is forbidden for read and write
		// SUB : You must never do a read or a write operation without going through poll() (or equivalent).
	}
	catch (std::exception &e)
	{
		std::stringstream ss;
		std::string statusMessage = Response::statusCodes.find(statusCode)->second;
		request.setMethodMimeType("test.html");

		ss << "<!DOCTYPE html>\n<html>\n<head><title>" << statusCode << " " << statusMessage << "</title></head>\n";
		ss << "<body>\n<center><h1>" << statusCode << " " << statusMessage << "</h1></center>\n";
		ss << "<hr><center>" << "WEBSERV OR SERVERNAME?" << "</center>\n</body>\n</html>\n";
		std::string body = ss.str();
		std::string totalString = createHeaderAndBodyString(request, body, statusCode);
		writeReturn = write(socketFd, totalString.c_str(), totalString.size());
		if (writeReturn == -1)
			throw std::runtime_error("Error writing to socket in Response::FallbackError!!");
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
