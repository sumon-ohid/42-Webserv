#ifndef HEADER_HPP
#define HEADER_HPP

#include "Method.hpp"

#define CLIENT_REQUEST 1
#define SERVER_RESPONSE 2

class Header {
	private:
		bool	_firstLineChecked;
		int		_type; // request or answer
		Method	_method;

	public:
		Header();
		Header(std::string method);
		Header(const Header& other);
		Header& operator=(const Header& other);
		~Header();

		std::string getMethodName();
		std::string getMethodPath();
		std::string getMethodProtocol();

		bool	getFirstLineChecked();

		void	setFirstLineChecked();

};

#endif