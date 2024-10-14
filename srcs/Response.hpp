#pragma once

#include <string>
#include <map>

#include "Request.hpp"
// #include "Socket.hpp"
#include "ErrorHandle.hpp"

#define CHUNK_SIZE 100000

class Response {
	private:
		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

	public:
		static const std::map<std::string, std::string> statusCodes;

		static std::string getActualTimeString();

		static void header(int socketFd, Request& request, std::string& body);
		static void	headerAndBody(int socketFd, Request& request, std::string& body);
		static void FallbackError(int socketFd, Request& request, std::string statusCode);

		static void FallbackError(int socketFd, Request& request, std::string statusCode, Client *client);

		static void	sendChunks(int socketFd, std::string chunkString);
		static void	sendWithChunkEncoding(int socketFd, Request& request, std::string& body);
};
