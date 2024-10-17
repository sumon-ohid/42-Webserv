#pragma once

#include <map>
#include <vector>

#include "Method.hpp"
#include "ServerConfig.hpp"

class Response;

class Request {
	private:
		bool	_firstLineChecked;
		bool	_readingFinished;
		int		_type; // request or response
		Method*	_method;
		std::map<std::string, std::string> _headerMap;


		void	checkOneLine(std::string oneLine);

	public:
		Response*	_response;
	
		Request();
		Request(std::string method);
		Request(const Request& other);
		Request&	operator=(const Request& other);
		bool		operator==(const Request& other) const;
		~Request();

		std::string getMethodName() const;
		std::string getMethodPath() const;
		std::string getMethodProtocol() const;
		std::string getMethodMimeType() const;
		bool		getFirstLineChecked() const;
		bool		getReadingFinished() const;
		std::map<std::string, std::string> getHeaderMap() const;

		void	setMethodMimeType(std::string path);

		void	checkFirstLine(std::vector<char>& line);
		void	checkLine(std::vector<char>& line);
		void	checkHost(ServerConfig& config) const;
		void	executeMethod(int socketFd, Client *client);

		void	requestReset();

};
