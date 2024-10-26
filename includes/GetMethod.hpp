#pragma once

class Request;
class Client;
class ServerConfig;

#include "Method.hpp"
#include "Client.hpp"
#include "LocationFinder.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Server.hpp"

#include <string>

class GetMethod : public Method
{
	private:
		int socketFd;

	public:
		GetMethod();
		GetMethod(const GetMethod& other);
		GetMethod& operator=(const GetMethod& other);
		~GetMethod();

		void executeMethod(int socketFd, Client *client, Request &request);
		void handleAutoIndexOrError(LocationFinder &locationFinder, Request& request, Client* client);
		void handleRedirection(std::string &redirectUrl);
		void handleAutoIndex(std::string &path, Request &request, Client *client);
		void serveStaticFile(LocationFinder &locationFinder, std::string &path, Request &request, Client *client);
		void executeCgiScript(std::string &requestPath, Client *client, Request &request);

		Method*	clone();
};
