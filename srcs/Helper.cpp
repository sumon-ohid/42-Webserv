#include "../includes/Helper.hpp"

#include <iomanip>
#include <sstream>
#include <ctime>
#include <sys/epoll.h>


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
	codes["413"] = "Payload Too Large";
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

static std::map<std::string, std::string> initMimeMap() {
	std::map<std::string, std::string> mimes;
	mimes[".css"] = "text/css";
	mimes[".html"] = "text/html";
	mimes[".js"] = "text/javascript";
	mimes[".jpg"] = "image/jpeg";
	mimes[".jpeg"] = "image/jpeg";
	mimes[".png"] = "image/png";
	mimes[".gif"] = "image/gif";
	mimes[".pdf"] = "application/pdf";
	mimes["mp4"] = "video/mp4";
	mimes["xml"] = "application/xml";
	mimes["json"] = "application/json";
	return mimes;
}

static std::map<std::string, std::string> initRedirectCodesMap() {
	std::map<std::string, std::string> redirect;
	redirect["301"] = "Moved Permanently";
	redirect["302"] = "Found"; //-- default if no code is specified
	redirect["303"] = "See Other"; //-- redirect after a POST request
	redirect["307"] = "Temporary Redirect";
	redirect["308"] = "Permanent Redirect";
	return redirect;
}

const std::map<std::string, std::string> Helper::statusCodes = initStatusCodesMap();
const std::map<std::string, std::string> Helper::executableMap = initExecMap();
const std::map<std::string, std::string> Helper::mimeTypes = initMimeMap();
const std::map<std::string, std::string> Helper::redirectCodes = initRedirectCodesMap();

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

void	Helper::modifyEpollEvent(Epoll &epoll, Client *client, uint32_t events)
{
	struct	epoll_event	event;
	event.events = events;
	event.data.fd = client->getFd();
	if (epoll_ctl(epoll.getFd(), EPOLL_CTL_MOD, event.data.fd, &event) == -1)
	{
		// return to client that there was an internal server error - (is that possible? we would have to change the event then to EPOLLOUT to be able to send the response to the client; maybe this was the one not working, we don't know for sure; but even if not, we would have to change the events, which failed before)
		// then remove client (?)
		epoll.removeClient(client);
	}
}
