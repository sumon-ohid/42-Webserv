#pragma once

class Request;
class Client;
class ServerConfig;

#include "Method.hpp"
#include "Client.hpp"
#include "LocationConfig.hpp"
#include "Response.hpp"
#include "Request.hpp"
#include "Server.hpp"

#include <vector>
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
		void handleAutoIndexOrError( std::string& root,  std::string& requestPath, bool autoIndex, Request& request, Client* client);
		void handleTryFiles( std::string& tryFiles,  std::string& root,  std::string& requestPath, Request& request, Client* client);
		void handleFileRequest( std::string& locationPath,  std::string& root,  std::string& requestPath,  std::string& index, Request& request, Client* client);
		void handleDirectoryRequest(std::string& locationPath,  std::string& root,  std::string& requestPath,  std::string& index, bool autoIndex,  std::string& tryFiles, Request& request, Client* client);
		void handleRequest( std::string& locationPath,  std::string& root,  std::string& requestPath,  std::string& index, bool autoIndex,  std::string& tryFiles, Request& request, Client* client);
		bool findMatchingLocation(std::vector<LocationConfig> &locationConfig, std::string &requestPath,
				std::string &locationPath, std::string &root, std::string &index, bool &cgiFound, bool &autoIndex,
				std::string &tryFiles);	
		void handleRedirection(std::string &redirectUrl);
		void handleAutoIndex(std::string &path, Request &request, Client *client);
		void serveStaticFile(std::string &path, Request &request, Client *client);
		void executeCgiScript(std::string &requestPath, Client *client, Request &request);

		Method*	clone();
};
