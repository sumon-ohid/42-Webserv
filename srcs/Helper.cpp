#include "../includes/Helper.hpp"

#include <iomanip>
#include <sstream>
#include <ctime>


Helper::Helper() {}

Helper::Helper(const Helper& other) {
	(void) other;
}

Helper& Helper::operator=(const Helper& other) {
	if (this == &other)
		return *this;
	// copy variables here
	return *this;
}

Helper::~Helper() {}

static std::map<std::string, std::string> initStatusCodesMap() {
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

static std::map<std::string, std::string> initExecMap() {
	std::map<std::string, std::string> execs;
	execs[".py"] = "/usr/bin/python3";
	execs[".php"] = "/usr/bin/php";
	execs[".sh"] = "/usr/bin/bash";
	return execs;
}

const std::map<std::string, std::string> Helper::statusCodes = initStatusCodesMap();
const std::map<std::string, std::string> Helper::executableMap = initExecMap();

std::string Helper::getActualTimeStringGMT() {
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
