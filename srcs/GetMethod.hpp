#pragma once

class Request;
class Client;
class ServerConfig;

#include "Method.hpp"
#include "Client.hpp"
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

class GetMethod : public Method
{
	private:
		// Add member variables here
		int socketFd;

	public:
		GetMethod();
		GetMethod(const GetMethod& other);
		GetMethod& operator=(const GetMethod& other);
		~GetMethod();

		void executeMethod(int socketFd, Client *client, Request &request);
		bool findMatchingLocation(std::vector<LocationConfig> &locationConfig, std::string &requestPath,
				std::string &locationPath, std::string &root, std::string &index, bool &cgiFound, bool &autoIndex);
		void handleRedirection(std::string &redirectUrl);
		void handleAutoIndex(std::string &path, Request &request, Client *client);
		void serveStaticFile(std::string &path, Request &request, Client *client);
		void executeCgiScript(std::string &requestPath, Client *client, Request &request);
		
		Method*	clone();
};
