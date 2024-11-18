#pragma once

// #include "Request.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>

#include "ErrorHandle.hpp"

class Request;

#define CHUNK_SIZE 64000

class Response {
	private:
		bool			_isChunk;
		bool			_headerSent;
		bool			_finishedSending;
		bool			_closeConnection;
		std::string		_header;
		std::string		_body;
		std::string		_sessionId; //-- BONUS : cookies
		ssize_t			_bytesSent;

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

		void	sendResponse(Client* client);

		void	fallbackError(Request& request, std::string statusCode, Client* client);
		void	error(Request& request, std::string statusCode, Client *client);

		void	prepareChunk(Client*);
		void	sendContentChunk(Client* client);
		void	sendNullChunk(Client* client);
		void	sendChunk(Client*);

		void	sendSimpleResponse(Client*);

		void	setIsChunk(bool);
		void	addToBody(const std::string&);
};
