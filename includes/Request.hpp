#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>

#include "Method.hpp"
#include "ServerConfig.hpp"

class Server;
class Response;

class Request {
	private:
		bool	_firstLineChecked;
		bool	_headerChecked;
		bool	_readingFinished;
		bool	_isChunked;
		int		_type; // request or response
		Method*	_method;
		std::map<std::string, std::string> _headerMap;
		std::size_t	_contentLength;
		std::size_t	_contentRead;
		void	storeOneHeaderInMap(const std::string& oneLine);
		void	storeHeadersInMap(const std::string& oneLine, std::size_t& endPos);

	public:
		std::string	_requestBody;
		std::string	_postFilename;

		Response*	_response;

		Request();
		Request(std::string method);
		Request(const Request& other);
		Request&	operator=(const Request& other);
		bool		operator==(const Request& other) const;
		~Request();

		bool		hasMethod() const;
		std::string getMethodName() const;
		std::string getMethodPath() const;
		std::string getMethodProtocol() const;
		std::string getMethodMimeType() const;
		bool		getFirstLineChecked() const;
		bool		getReadingFinished() const;
		std::map<std::string, std::string> getHeaderMap() const;

		void	setMethodMimeType(std::string path);
		void	storeRequestBody(const std::string& strLine, std::size_t endPos);
		void	extractHttpMethod(std::string&);
		void	createHttpMethod(const std::string&);
		void	checkFirstLine(std::string& strLine, std::size_t& endPos);

		void	checkLine(std::vector<char>& line);
		void	checkHost(ServerConfig& config) const;
		void	executeMethod(int socketFd, Client *client);

		int		clientRequest(Client* client);
		void	validRequest(Server* serv, std::vector<char> buffer, ssize_t count, Request& request);
		void	validRequest(std::vector<char>, ssize_t, Request&);
		int		invalidRequest(Client*);
		int		emptyRequest(Client*);

		void	requestReset();

};
