#pragma once

#include <vector>
#include <map>

#include "Method.hpp"
#include "ServerConfig.hpp"

class Request {
	private:
		bool	_firstLineChecked;
		bool	_readingFinished;
		int		_type; // request or response
		Method*	_method;
		std::map<std::string, std::string> _headerMap;

		void	checkOneLine(std::string oneLine);

	public:
		Request();
		Request(std::string method);
		Request(const Request& other);
		Request& operator=(const Request& other);
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
		void	executeMethod(int socketFd, ServerConfig config);

		void	requestReset();

};
