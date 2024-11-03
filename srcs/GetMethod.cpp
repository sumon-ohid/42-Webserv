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

GetMethod::GetMethod() : Method() { socketFd = -1; }

GetMethod::GetMethod(const GetMethod& other) : Method(other) {}

GetMethod&	GetMethod::operator=(const GetMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
	return *this;
}

GetMethod::~GetMethod() {}

std::string holdLocationPath;
std::string matchLocationPath;

//-- This function can execute the request.
//-- Store the request path,
//-- compare it with location path.
//-- if match get root and index and other values.
void GetMethod::executeMethod(int _socketFd, Client* client, Request& request)
{
    socketFd = _socketFd;
    std::string pathToServe;
    bool locationMatched = false;
    std::string requestPath = request.getMethodPath();

    LocationFinder locationFinder;
    locationMatched = locationFinder.locationMatch(client, requestPath, _socketFd);
    if (locationMatched)
    {
        if (locationFinder._redirectFound)
        {
            handleRedirection(locationFinder._redirect);
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
    std::string fullPath =locationFinder._root + locationFinder._locationPath;
    if (locationFinder._autoIndex == "on" && locationFinder.isDirectory(fullPath))
        handleAutoIndex(fullPath, request, client);
    else
        request._response->error(request, "403", client);
}

void GetMethod::handleAutoIndex(std::string &path, Request &request, Client *client)
{
    DIR *dir;
    struct dirent *ent;
    std::ostringstream body;

    if ((dir = opendir(path.c_str())) != NULL)
    {
        body << "<html><head><title>Index of "
        << path << "</title></head><body><h1>Index of "
        << path << "</h1><hr><pre>";

        while ((ent = readdir(dir)) != NULL)
        {
            body << "<a href=\""
                << "/" << ent->d_name
                << "\">" << ent->d_name
                << "</a><br>";
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

void GetMethod::handleRedirection(std::string &redirectUrl)
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

    std::cout << BOLD YELLOW << "Redirecting to: " << url << RESET << std::endl; // BP: maybe move this part to response class
    std::ostringstream redirectHeader;
    redirectHeader << "HTTP/1.1 " << redirectCode << " " << redirectCodes[redirectCode] << "\r\n"
                   << "Location: " << url << "\r\n"
                   << "Content-Length: 0\r\n"
                   << "Connection: close\r\n\r\n";
    std::string response = redirectHeader.str();
    ssize_t bytes_written = write(socketFd, response.c_str(), response.size());
    if (bytes_written == -1)
        throw std::runtime_error("Error writing to socket in GetMethod::handleRedirection!!");
    else if (bytes_written == 0)
        std::cerr << BOLD RED << "Error: 0 bytes written to socket in GetMethod::handleRedirection" << RESET << std::endl;
    else
        std::cout << BOLD GREEN << "Redirect response sent successfully" << RESET << std::endl;
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

    this->setMimeType(path);
    std::ifstream file(path.c_str());
    if (locationFinder.isDirectory(path))
    {
        path = path + "/";
        handleAutoIndexOrError(locationFinder, request, client);
        return;
    }
    if (!file.is_open())
    {
        //std::cerr << BOLD RED << "Error: 404 not found" << RESET << std::endl;
        request._response->error(request, "404", client);
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();
    file.close();
    request._response->createHeaderAndBodyString(request, body, "200", client);
    std::cout << request.getMethodName() << " " << request.getMethodPath() << RESET << std::endl;
    std::cout << BOLD GREEN << "Response sent to client successfully 🚀" << RESET << std::endl;
}

//-- Handle CGI script execution.
void GetMethod::executeCgiScript(std::string &requestPath, Client *client, Request &request)
{
    try
    {
        client->_cgi = HandleCgi(requestPath, socketFd, *client, request);
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
