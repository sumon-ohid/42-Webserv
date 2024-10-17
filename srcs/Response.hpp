#pragma once

// #include "Request.hpp"
#include <string>
#include <map>

#include "ErrorHandle.hpp"
class Request;

#define CHUNK_SIZE 100000

class Response {
	private:
		int 			_socketFd;
		bool			_isChunk;
		unsigned long	_bytesSent;
		std::string		_message;
		// std::string chunk;
		std::string		_mimeType;


	public:
		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

		static const std::map<std::string, std::string> statusCodes; //BP maybe move from here?

		static std::string getActualTimeString();

		Response* clone() const;


		static void header(int socketFd, Request& request, std::string& body);
		static void	headerAndBody(int socketFd, Request& request, std::string& body);
		static void FallbackError(int socketFd, Request& request, std::string statusCode);

		static void FallbackError(int socketFd, Request& request, std::string statusCode, Client *client);

		static void	sendChunks(int socketFd, std::string chunkString);
		static void	sendWithChunkEncoding(int socketFd, Request& request, std::string& body);
};
