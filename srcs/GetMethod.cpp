#include "GetMethod.hpp"
#include "Method.hpp"
// #include "main.hpp"
#include "HandleCgi.hpp"
#include "LocationConfig.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"

#include <iostream>
#include <cstddef>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <string>

GetMethod::GetMethod() : Method() {}

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

//-- NOTE: This function is too long. It should be broken down into smaller functions.

void GetMethod::executeMethod(int socketFd, Client *client, Request &request)
{
    std::vector<LocationConfig> locationConfig = client->_server->_serverConfig.getLocations();
    std::string requestPath = request.getMethodPath();
    std::string locationPath;
    std::string root;
    std::string index;
    std::string body;
    std::string path;
    bool locationMatched = false;

    //--- Loop through the locationConfig vector
    for (size_t i = 0; i < locationConfig.size(); i++)
    {
        std::string tempPath = locationConfig[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        //-- Match only if the request path exactly matches the location path or
		//-- starts with the location path followed by a '/'
        if (requestPath == tempPath || (requestPath.find(tempPath) == 0 && requestPath[tempPath.length()] == '/'))
        {
            locationMatched = true;
            //-- Get the locationMap and concatenate root+index in locationPath
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;

            for (it = locationMap.begin(); it != locationMap.end(); it++)
            {
                //--- Handle other keys as well.
                if (it->first == "root")
                    root = it->second;
                if (it->first == "index")
                    index = it->second;
                if (it->first == "return")
                {
                    //-- Send HTTP 301 response for redirection
                    std::string redirectUrl = it->second;
                    std::cout << BOLD YELLOW << "Redirecting to: " << redirectUrl << RESET << std::endl;
                    std::ostringstream redirectHeader;
                    redirectHeader << "HTTP/1.1 301 Moved Permanently\r\n"
                        << "Location: " << redirectUrl << "\r\n"
                        << "Content-Length: 0\r\n"
                        << "Connection: close\r\n\r\n";
                    std::string response = redirectHeader.str();
                    ssize_t bytes_written = write(socketFd, response.c_str(), response.size());
                    if (bytes_written == -1)
                        throw std::runtime_error("Error writing to socket in GetMethod::executeMethod!!");
                    else
                        std::cout << BOLD GREEN << "Redirect response sent successfully" << RESET << std::endl;
                    return;
                }
            }
            if (requestPath == "/")
                locationPath = root + index;
            else
                locationPath = root + requestPath;
            path = locationPath;

			// std::cout << BOLD BLUE << requestPath << RESET << std::endl;
			// std::cout << BOLD BLUE << tempPath << RESET << std::endl;

			//--- Set the MIME type of the file
            this->setMimeType(path);
            //--- Check if the file exists or not
            std::ifstream file(path.c_str());
            if (!file.is_open())
            {
                Response::FallbackError(socketFd, request, "404");
                return;
            }

            //--- Read the file and send it to the client
            std::ostringstream buffer;
            buffer << file.rdbuf();
            body = buffer.str();
            file.close();
            Response::headerAndBody(socketFd, request, body);
            return;
        }
    }

    //-- If no matching location is found, eg: css, js, etc.
    if (!locationMatched)
    {
        for (size_t i = 0; i < locationConfig.size(); i++)
        {
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it = locationMap.find("root");
            if (it != locationMap.end())
                root = it->second;
            path = root + request.getMethodPath();

            this->setMimeType(path);
            std::ifstream file(path.c_str());
            if (!file.is_open())
            {
                Response::FallbackError(socketFd, request, "404");
                return;
            }
            std::ostringstream buffer;
            buffer << file.rdbuf();
            body = buffer.str();
            file.close();
            Response::headerAndBody(socketFd, request, body);

        }
    }
}

Method*	GetMethod::clone() {
	return new GetMethod(*this);
}
