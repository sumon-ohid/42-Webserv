#pragma once

#include <vector>
#include <map>

#include "Method.hpp"

#define CLIENT_REQUEST 1
#define SERVER_RESPONSE 2

class Header {
	private:
		bool	_firstLineChecked;
		bool	_readingFinished;
		int		_type; // request or response
		Method*	_method;
		std::map<std::string, std::string> headerMap;

	public:
		Header();
		Header(std::string method);
		Header(const Header& other);
		Header& operator=(const Header& other);
		~Header();

		std::string getMethodName();
		std::string getMethodPath();
		std::string getMethodProtocol();
		bool		getFirstLineChecked();
		bool		getReadingFinished();

		void	checkFirstLine(std::vector<char>& line);
		void	checkLine(std::vector<char>& line);

		void	headerReset();

};
