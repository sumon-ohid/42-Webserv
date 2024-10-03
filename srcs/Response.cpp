#include <sstream>
#include <ctime>
#include <iomanip>
#include <iostream>

#include "Response.hpp"

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

static std::string createHeaderString(Header& header, std::string& body) {
	std::stringstream ss;

	ss << header.getMethodProtocol() << " " << "200" << " " << "OK" << "\n"; // or use string directly
	ss << "Server: " << "someName" << "\n";
	ss << "Date: " << Response::getActualTimeString() << "\n";
	ss << "Content-Type: " << "someContentType" << "\n";
	ss << "Content-Length: " << (body.size() + 1) << "\n"; // + 1 plus additional \n at the end?
	ss << "Connection: " << "connectionClosedOrNot" << "\n";

	return ss.str();
}

void	Response::header(int socketFd, Header& header, std::string& body) {
	std::string headString = createHeaderString(header, body);
	write(socketFd , headString.c_str(), headString.size());
}

void	Response::headerAndBody(int socketFd, Header& header, std::string& body) {
	std::stringstream ss;
	ss << createHeaderString(header, body) << "\n";
	ss << body << "\n"; //BP: \n at the end - do we need this?
	std::string totalString = ss.str();

	write(socketFd , totalString.c_str(), totalString.size());
}
