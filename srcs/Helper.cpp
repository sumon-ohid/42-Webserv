#include "../includes/Helper.hpp"
#include "../includes/Response.hpp"

#include <cstddef>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <stdexcept>
#include <sys/epoll.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>

Helper::Helper() {}

Helper::Helper(const Helper& other) {
	(void) other;
}

Helper& Helper::operator=(const Helper& other) {
	if (this == &other)
		return *this;

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
	codes["303"] = "See Other";
	codes["307"] = "Temporary Redirect";
	codes["308"] = "Permanent Redirect";
	codes["400"] = "Bad Request";
	codes["401"] = "Unauthorized";
	codes["403"] = "Forbidden";
	codes["404"] = "Not Found";
	codes["405"] = "Not Allowed";
	codes["413"] = "Payload Too Large";
	codes["415"] = "Unsupported Media Type";
	codes["500"] = "Internal Server Error";
	codes["503"] = "Service Unavailable";
	codes["504"] = "Gateway Timeout";
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
	mimes[".bmp"] = "image/bmp";
	mimes[".css"] = "text/css";
	mimes[".csv"] = "text/csv";
	mimes[".gif"] = "image/gif";
	mimes[".html"] = "text/html";
	mimes[".ico"] = "image/png";
	mimes[".ics"] = "text/calendar";
	mimes[".jpeg"] = "image/jpeg";
	mimes[".jpg"] = "image/jpeg";
	mimes[".js"] = "text/javascript";
	mimes[".json"] = "application/json";
	mimes[".mp3"] = "audio/mpeg";
	mimes[".mp4"] = "video/mp4";
	mimes[".otf"] = "font/otf";
	mimes[".pdf"] = "application/pdf";
	mimes[".png"] = "image/png";
	mimes[".php"] = "application/x-httpd-php";
	mimes[".svg"] = "image/svg+xml";
	mimes[".ttf"] = "font/ttf";
	mimes[".txt"] = "text/plain";
	mimes[".wav"] = "audio/wav";
	mimes[".woff"] = "font/woff";
	mimes[".woff2"] = "font/woff2";
	mimes[".webmanifest"] = "application/json";
	mimes[".xhtml"] = "application/xhtml+xml";
	mimes[".xml"] = "application/xml";

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

void	Helper::checkStatus(std::string& statusCode, std::string& statusMessage) {
	std::map<std::string, std::string>::const_iterator it;

	it = Helper::statusCodes.find(statusCode);
	if (it == Helper::statusCodes.end()) {
		std::cout << "status code in check Status: " << statusCode << std::endl;
		statusCode = "500";
		statusMessage = "Internal Server Error";
	} else {
		statusMessage = it->second;
	}
}

void	Helper::modifyEpollEventClient(Epoll &epoll, Client *client, uint32_t events)
{
	struct	epoll_event	event;
	std::memset(&event, 0, sizeof(event)); // Zero-initialize the epoll_event structure
	event.events = events;
	event.data.fd = client->getFd();
	if (epoll_ctl(epoll.getFd(), EPOLL_CTL_MOD, event.data.fd, &event) == -1)
		throw std::runtime_error("500");
		// epoll.removeClient(client);
}

void	Helper::addFdToEpoll(Client* client, int fd, uint32_t event)
{
	if (!client->_epoll->registerSocket(fd, event))
		return ;
    client->_epoll->addCgiClientToEpollMap(fd, client);
}

void	Helper::prepareIO(Client* client, int fd, std::string& path, std::string mode)
{
	client->_io.setSize(Helper::checkFileSize(path, client));
	client->_io.setFd(fd);
	client->_epoll->addClientIo(client, mode);
	if (mode == "read")
		client->_request.begin()->_isRead = true;
	else if (mode == "write")
		client->_request.begin()->_isWrite = true;
	else
		throw std::runtime_error("500");
}

std::string Helper::decodeUrl(std::string url)
{
    std::string decodedUrl;
    std::stringstream ss;

    for (size_t i = 0; i < url.length(); i++)
	{
		if (url[i] == '%')
		{
            if (i + 2 < url.length())
			{
                std::string temp = url.substr(i + 1, 2);
                int decimal;
                ss.clear();
                ss.str(temp);
                ss >> std::hex >> decimal;
                decodedUrl += static_cast<char>(decimal);
                i += 2;
			}
            else
                decodedUrl += '%';
        }
		else if (url[i] == '+')
			decodedUrl += ' ';
		else
            decodedUrl += url[i];
    }
    return decodedUrl;
}

long	Helper::checkFileSize(const std::string& path, Client* client)
{
	struct stat fileStat;

	if (stat(path.c_str(), &fileStat) == -1)
		client->_request.back()._response->error(client->_request.back(), mapErrnoToHttpCodeString(), client);
	return (fileStat.st_size);
}

std::string	Helper::mapErrnoToHttpCodeString() {
	switch (errno) {
		case ENOENT: return "404"; // Not Found
		case EACCES: return "403"; // Forbidden
		case ENAMETOOLONG: return "414"; // URI Too Long
		case ENOTDIR: return "404"; // Not Found
		default: return "500"; // Internal Server Error
	}
}

// BONUS : cookies
std::string Helper::generateRandomId()
{
	std::string sessionId = "";
	std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	for (int i = 0; i < 32; i++)
	{
		sessionId += chars[std::rand() % chars.size()];
	}
	return sessionId;
}

void	Helper::setCloexec(int fd)
{
	int flags = fcntl(fd, F_GETFD);
	if (flags == -1)
		throw std::runtime_error("500");
	flags |= FD_CLOEXEC;
	if (fcntl(fd, F_SETFD, flags) == -1)
		throw std::runtime_error("500");
}

void	Helper::setFdFlags(int fd, uint32_t mask)
{
	int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        throw std::runtime_error("500");
    }
    flags |= mask;
    if (fcntl(fd, F_SETFL, flags) == -1) {
        throw std::runtime_error("500");
    }
}

double	Helper::getElapsedTime(Client *client)
{
	time_t	currentTime = std::time(NULL);
	return (std::difftime(currentTime, client->getLastActive()));
}

void	Helper::toLower(std::string& str) {
	for (std::string::iterator it = str.begin(); it != str.end(); it++) {
		*it = std::tolower(*it);
	}
}
