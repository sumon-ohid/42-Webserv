#pragma once

#include <vector>
#include <map>

#include "Method.hpp"

class Request {
	private:
		bool	_firstLineChecked;
		bool	_readingFinished;
		int		_type; // request or response
		Method*	_method;
		std::map<std::string, std::string> requestMap;

	public:
		Request();
		Request(std::string method);
		Request(const Request& other);
		Request& operator=(const Request& other);
		~Request();

		std::string getMethodName();
		std::string getMethodPath();
		std::string getMethodProtocol();
		bool		getFirstLineChecked();
		bool		getReadingFinished();

		void	checkFirstLine(std::vector<char>& line);
		void	checkLine(std::vector<char>& line);

		void	requestReset();

};
