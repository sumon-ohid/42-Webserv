#pragma once

// #include "Request.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>

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
		std::string		_body;
		std::string		_mimeType;
		ssize_t			_bytesSent;
		size_t			_totalBytesSent;

	public:
		Response();
		Response(const Response& other);
		Response& operator=(const Response& other);
		~Response();

		Response* clone() const;

		bool	getIsChunk();
		bool	getIsFinished();
		size_t	getBodySize() const;

		std::string	createHeaderString(Request& request, const std::string& body, std::string statusCode);
		void		createHeaderAndBodyString(Request& request, std::string& body, std::string statusCode, Client* client);

		void	sendResponse(Client* client, int socketFd, Request& request);

		void	fallbackError(Request& request, std::string statusCode, Client* client);
		void	error(Request& request, std::string statusCode, Client *client);

		void	sendChunkHeader(Client*, int socketFd, Request& request);
		long	sendChunks(Client* client, std::string& chunkString);

		void	setIsChunk(bool);
		void	addToBody(const std::string&);
};