#pragma once

// #include "Request.hpp"
#include <string>

#include "ErrorHandle.hpp"

class Request;

#define CHUNK_SIZE 100000

class Response {
	private:
		int 			_socketFd;
		bool			_isChunk;
		bool			_headerSent;
		bool			_finishedSending;
		bool			_closeConnection;
		unsigned long	_bytesSentOfBody;
		std::string		_header;
		std::string		_message;
		std::string		_mimeType;

	public:
		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

		Response* clone() const;

		bool	getIsChunk();

		std::string	createHeaderString(Request& request, const std::string& body, std::string statusCode);
		void		createHeaderAndBodyString(Request& request, std::string& body, std::string statusCode);

		void	header(int socketFd, Request& request, std::string& body);
		void	headerAndBody(Client*, int socketFd, Request& request, std::string& body);
		void	fallbackError(int socketFd, Request& request, std::string statusCode);

		void	error(int socketFd, Request& request, std::string statusCode, Client *client);

		long	sendChunks(int socketFd, std::string chunkString);
		void	sendWithChunkEncoding(Client*, int socketFd, Request& request);
};
