#include "../includes/Client.hpp"
#include "../includes/GetMethod.hpp"
#include "../includes/Method.hpp"
#include "../includes/HandleCgi.hpp"
#include "../includes/Response.hpp"
#include "../includes/ServerConfig.hpp"
#include "../includes/LocationFinder.hpp"
#include "../includes/Helper.hpp"

#include <iostream>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sstream>
#include <dirent.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <fcntl.h>

GetMethod::GetMethod() : Method() { _socketFd = -1; }

GetMethod::GetMethod(const GetMethod& other) : Method(other), _socketFd(other._socketFd) {}

GetMethod&	GetMethod::operator=(const GetMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
    _socketFd = other._socketFd;
	return *this;
}

GetMethod::~GetMethod() {}

//-- This function can execute the request.
//-- Store the request path,
//-- compare it with location path.
//-- if match get root and index and other values.
void GetMethod::executeMethod(int socketFd, Client* client, Request& request)
{
    _socketFd = socketFd;
    std::string pathToServe;
    bool locationMatched = false;
    std::string requestPath = request.getMethodPath();

    LocationFinder locationFinder;
    if (requestPath.find("$autoFlag=on") != std::string::npos) {
        locationFinder._autoIndexMode = true;
        std::size_t pos = requestPath.find("$autoFlag=on");
        requestPath.erase(pos);
    }
    locationMatched = locationFinder.locationMatch(client, requestPath, _socketFd);
    if (locationMatched)
    {
        if (locationFinder._redirectFound)
        {
            handleRedirection(client, request, locationFinder._redirect);
            return;
        }
        if (locationFinder._cgiFound)
        {
            pathToServe = locationFinder._pathToServe;

            if (locationFinder.isDirectory(pathToServe))
            {
                handleAutoIndexOrError(locationFinder ,request, client);
                return;
            }
            executeCgiScript(requestPath, client, request);
            return;
        }
        pathToServe = locationFinder._pathToServe;
        std::ifstream file(pathToServe.c_str());
        if (!file.is_open())
        {
            handleAutoIndexOrError(locationFinder ,request, client);
            return;
        }
        file.close();
        serveStaticFile(locationFinder, pathToServe, request, client);
    }
    else
    {
        pathToServe = locationFinder._pathToServe;
        serveStaticFile(locationFinder, pathToServe, request, client);
    }
}

void GetMethod::handleAutoIndexOrError(LocationFinder &locationFinder, Request& request, Client* client)
{
    std::string fullPath = locationFinder._pathToServe;
    if (!locationFinder.isDirectory(fullPath))
       fullPath = locationFinder._root + locationFinder._locationPath;

    if (locationFinder._autoIndex == "on" && locationFinder.isDirectory(fullPath))
        handleAutoIndex(fullPath, request, client);
    else
        request._response->error(request, "403", client);
}


//-- Function template to convert various types to string
template <typename T>
static std::string  anyToString(const T& value)
{
    std::stringstream ss;
    ss << value;
    return ss.str();
}

void GetMethod::handleAutoIndex(std::string &path, Request &request, Client *client)
{
    DIR *dir;
    struct dirent *ent;
    std::ostringstream body;

    size_t pos = path.find("//");
    if (pos != std::string::npos)
        path.erase(pos, 1);

    if ((dir = opendir(path.c_str())) != NULL)
    {
        body << "<html><head><title>Index of "
             << path << "</title></head><body><h1>Index of "
             << path << "</h1><hr><pre>";

        while ((ent = readdir(dir)) != NULL)
        {
            std::string fullPath = path + "/" + ent->d_name;
            struct stat fileStat;

            if (stat(fullPath.c_str(), &fileStat) == 0)
            {
                std::string file = ent->d_name;

                if (file == "." || file == "..")
                    continue;
                else
                {
                    if (S_ISDIR(fileStat.st_mode))
                        file += "/";

                    //-- Format file size
                    std::string size;
                    if (S_ISDIR(fileStat.st_mode))
                        size = "-";
                    else
                        size = anyToString(fileStat.st_size);

                    //-- Format last modified time
                    char timeBuffer[30];
                    strftime(timeBuffer, sizeof(timeBuffer), "%d-%b-%Y %H:%M", localtime(&fileStat.st_mtime));

                    //-- Make links for each file and directory
                    body << "<a href=\"";
                    if (S_ISDIR(fileStat.st_mode))
                        body << file; //-- Directory link
                    else
                        body << file; //-- File link with directory
                    body << "$autoFlag=on\">" << std::setw(30) << std::left << file << "</a>"
                         << std::setw(30) << std::left << timeBuffer
                         << size
                         << "<br>";
                }
            }
        }
        body << "</pre><hr></body></html>";
        closedir(dir);
    }
    else
    {
        request._response->error(request, "403", client);
        return;
    }
    std::string bodyStr = body.str();
    request._response->createHeaderAndBodyString(request, bodyStr, "200", client);
    std::cout << BOLD GREEN << "Autoindex response sent to client successfully 🚀" << RESET << std::endl;
}


void GetMethod::handleRedirection(Client* client, Request& request, std::string &redirectUrl)
{
    std::map<std::string, std::string> redirectCodes = Helper::redirectCodes;

    std::string redirectCode = "302"; // Default redirect code
    std::string url = redirectUrl;

    //-- Check if the redirectUrl contains a redirect code at the beginning
    size_t spacePos = redirectUrl.find(" ");
    if (spacePos != std::string::npos)
    {
        std::string potentialCode = redirectUrl.substr(0, spacePos);
        if (redirectCodes.find(potentialCode) != redirectCodes.end())
        {
            redirectCode = potentialCode;
            url = redirectUrl.substr(spacePos + 1);
        }
    }
    std::cout << BOLD YELLOW << "Redirecting to: " << url << RESET << std::endl;
    request._response->createHeaderAndBodyString(request, url, redirectCode, client);
}

void GetMethod::serveStaticFile(LocationFinder &locationFinder, std::string &path, Request &request, Client *client)
{
    signal (SIGPIPE, SIG_IGN);
    //-- Check if the allowed methods include GET
    //-- If not, return 405 Method Not Allowed
    //-- It will check in the matched location block
    if (locationFinder._allowedMethodFound)
    {
        if (locationFinder._allowed_methods.find("GET") == std::string::npos)
        {
            request._response->error(request, "405", client);
            return;
        }
    }
    std::ifstream file(path.c_str());
    if (locationFinder.isDirectory(path))
    {
        path = path + "/";
        handleAutoIndexOrError(locationFinder, request, client);
        return;
    }
	int fd = open(path.c_str(), O_NONBLOCK);
    std::cout << request._response->_methodAndPath << std::endl;
    std::cout << "fd: " << fd << std::endl;
	if (fd == -1)
        return (request._response->error(request, "404", client));
	setMimeType(path);
    Helper::prepareIO(client, fd, path, "read");
	if (client->_io.getSize() > CHUNK_SIZE)
		client->_request.back()._response->setIsChunk(true);
}

//-- Handle CGI script execution.
void GetMethod::executeCgiScript(std::string &requestPath, Client *client, Request &request)
{
    try
    {
        client->_cgi = HandleCgi(requestPath, _socketFd, *client, request);
        std::cout << BOLD GREEN << "CGI script executed successfully." << RESET << std::endl;
    }
    catch (std::exception &e)
    {
        request._response->error(request, e.what(), client);
    }
}

Method*	GetMethod::clone() {
	return new GetMethod(*this);
}
