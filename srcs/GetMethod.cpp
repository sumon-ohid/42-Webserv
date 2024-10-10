#include "GetMethod.hpp"
#include "HandleCgi.hpp"
#include "LocationConfig.hpp"
#include "main.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Server.hpp"
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
void	GetMethod::executeMethod(int socketFd, Client *client, Request &request) 
{
	// std::string bufferRead(buffer.begin(), buffer.end());
	// size_t pos = bufferRead.find("cgi-bin");
	// if (pos != std::string::npos)
	// 	HandleCgi cgi(request.getMethodPath(), _connSock, serv);

	std::vector<LocationConfig> locationConfig = client->_server->_serverConfig.getLocations();
	std::string requestPath = request.getMethodPath();
	std::string locationPath;
	std::string root;
	std::string index;
	std::string body;
	std::string path;


	//--- Loop through the locationConfig vector
	for(size_t i = 0; i < locationConfig.size(); i++)
	{
		std::string tempPath = locationConfig[i].getPath();
		tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
		tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());
		
		if (requestPath.find(tempPath) != std::string::npos)
		{
			//-- Get the locationMap and concatinate root+index in locationPath
			std::multimap<std::string, std::string > locationMap = locationConfig[i].getLocationMap();
			std::multimap<std::string, std::string >::iterator it;
			for (it = locationMap.begin(); it != locationMap.end(); it++)
			{
				//--- TODO: need to handle other keys aslo.
				if (it->first == "root")
					root = it->second;
				if (it->first == "index")
					index = it->second;
			}
			if (requestPath == "/")
				locationPath = root + index;
			else  
				locationPath = root + requestPath;
			std::cout << request.getMethodName() << " & " << locationPath << std::endl;

				path = locationPath;

			//--- Check if the file exists or not
			std::cout << "Config Path : " << path << std::endl;
			this->setMimeType(path);
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
		}
		else  
			Response::FallbackError(socketFd, request, "404");
	}

	// 	Response::header(socketFd, client->_request, body);
}
