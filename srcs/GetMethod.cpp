#include "GetMethod.hpp"
#include "Method.hpp"
#include "HandleCgi.hpp"
#include "ServerConfig.hpp"
#include "Response.hpp"

#include <iostream>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <algorithm>
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

//-- This function can execute the request.
//-- Store the request path,
//-- compare it with location path.
//-- if match get root and index and other values.
void GetMethod::executeMethod(int _socketFd, Client *client, Request &request)
{
    socketFd = _socketFd;
    std::vector<LocationConfig> locationConfig = client->_server->_serverConfig.getLocations();
    std::string requestPath = request.getMethodPath();
    std::string locationPath;
    std::string root;
    std::string index;
    std::string tryFiles;
    bool locationMatched = false;
    bool cgiFound = false;
    bool autoIndex = false;

    locationMatched = findMatchingLocation(locationConfig, requestPath,
                        locationPath, root, index, cgiFound, autoIndex, tryFiles);
    if (locationMatched)
    {
        if (cgiFound)
            executeCgiScript(requestPath, client, request);
        else if (requestPath[requestPath.length() - 1] == '/')
        {
            std::ifstream file(locationPath.c_str());
            if (!file.is_open() && autoIndex)
            {
                std::cout << BOLD YELLOW << "File not opened, Autoindex ON " << RESET << std::endl;
                std::string fullPath = root + requestPath;
                handleAutoIndex(fullPath, request, client);
            }
            else if (!file.is_open() && !autoIndex)
            {
                std::cout << BOLD YELLOW << "File not opened, Autoindex OFF " << RESET << std::endl;
                locationPath = "./conf/webpage/home.html";
                serveStaticFile(locationPath, request, client);
            }
            else
            {
                std::cout << BOLD YELLOW << "File opened, Autoindex OFF/ON " << RESET << std::endl;
                if (!tryFiles.empty() && (index.empty() || root.empty()))
                {
                    size_t found = tryFiles.find("./");
                    if (found == std::string::npos)
                        Response::error(socketFd, request, "404", client);
                    std::string tryFilesPath = tryFiles.substr(found);
                    std::ifstream tryFilesFile(tryFilesPath.c_str());
                    if (tryFilesFile.is_open())
                    {
                        locationPath = tryFilesPath;
                        serveStaticFile(locationPath, request, client);
                        return;
                    }
                    else
                        Response::error(socketFd, request, "404", client);
                }
                // else
                //    locationPath = root + requestPath + "/" + index;

                serveStaticFile(locationPath, request, client);
            }
            file.close();
        }
        else if (locationPath.find(".html") != std::string::npos)
        {
            if (requestPath.find(".css") != std::string::npos || requestPath.find(".js") != std::string::npos)
                locationPath = root + requestPath;
            else
                locationPath = root + requestPath + "/" + index;

            serveStaticFile(locationPath, request, client);
        }
        else
        {
            locationPath = root + requestPath + index;
            serveStaticFile(locationPath, request, client);
        }
    }
    else
    {
        for (size_t i = 0; i < locationConfig.size(); i++)
        {
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it = locationMap.find("root");
            if (it != locationMap.end())
                root = it->second;
            std::string path = root + request.getMethodPath();

            serveStaticFile(path, request, client);
        }
    }
}

bool GetMethod::findMatchingLocation(std::vector<LocationConfig> &locationConfig, std::string &requestPath,
     std::string &locationPath, std::string &root, std::string &index, bool &cgiFound, bool &autoIndex, std::string &tryFiles)
{
    for (size_t i = 0; i < locationConfig.size(); i++)
    {
        std::string tempPath = locationConfig[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());
        // if (!tempPath.empty() && tempPath[tempPath.length() - 1] != '/')
        //     tempPath += '/';

        if (requestPath == tempPath || (requestPath.find(tempPath) == 0 && requestPath[tempPath.length()] == '/'))
        {
            if (requestPath.find("cgi-bin") != std::string::npos)
            {
                cgiFound = true;
                return true;
            }

            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;

            for (it = locationMap.begin(); it != locationMap.end(); it++)
            {
                if (it->first == "root")
                    root = it->second;
                if (it->first == "index")
                    index = it->second;
                if (it->first == "return")
                {
                    handleRedirection(it->second);
                    return false;
                }
                if (it->first == "autoindex")
                {
                    if (it->second == "on")
                        autoIndex = true;
                }
                if (it->first == "try_files")
                    tryFiles = it->second;
            }
            if (requestPath == "/")
                locationPath = root + index;
            else if (autoIndex)
                locationPath = root + requestPath + index;
            else
                locationPath = root + requestPath;
            return true;
        }
    }
    return false;
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
        Response::error(socketFd, request, "403", client);
        return;
    }
    std::string bodyStr = body.str();
    Response::headerAndBody(socketFd, request, bodyStr);
    std::cout << BOLD GREEN << "Autoindex response sent to client successfully 🚀" << RESET << std::endl;
}

void GetMethod::handleRedirection(std::string &redirectUrl)
{
    std::cout << BOLD YELLOW << "Redirecting to: " << redirectUrl << RESET << std::endl;
    std::ostringstream redirectHeader;
    redirectHeader << "HTTP/1.1 301 Moved Permanently\r\n"
                   << "Location: " << redirectUrl << "\r\n"
                   << "Content-Length: 0\r\n"
                   << "Connection: close\r\n\r\n";
    std::string response = redirectHeader.str();
    ssize_t bytes_written = write(socketFd, response.c_str(), response.size());
    if (bytes_written == -1)
        throw std::runtime_error("Error writing to socket in GetMethod::handleRedirection!!");
    else
        std::cout << BOLD GREEN << "Redirect response sent successfully" << RESET << std::endl;
}

void GetMethod::serveStaticFile(std::string &path, Request &request, Client *client)
{
    signal (SIGPIPE, SIG_IGN);

    this->setMimeType(path);
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        //std::cerr << BOLD RED << "Error: 404 not found" << RESET << std::endl;
        Response::error(socketFd, request, "404", client);
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();
    file.close();
    Response::headerAndBody(socketFd, request, body);
    std::cout << BOLD GREEN << "Response sent to client successfully 🚀" << RESET << std::endl;
}

//-- Handle CGI script execution.
void GetMethod::executeCgiScript(std::string &requestPath, Client *client, Request &request)
{
    try
    {
        HandleCgi cgi(requestPath, socketFd, *client, request);
        std::cout << BOLD GREEN << "CGI script executed successfully." << RESET << std::endl;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << BOLD RED << "Error: " << e.what() << RESET << std::endl;
        Response::error(socketFd, request, "404", client);
    }
}

Method*	GetMethod::clone() {
	return new GetMethod(*this);
}
