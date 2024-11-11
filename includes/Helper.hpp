#pragma once

#include "Epoll.hpp"
#include <map>
#include <string>

class Helper {
	private:
		Helper();
		Helper(const Helper& other);
		Helper& operator=(const Helper& other);
		~Helper();

	public:
		// std::string statusCode, std::string statusMessage
		static const std::map<std::string, std::string> statusCodes;
		static const std::map<std::string, std::string> executableMap;
		static const std::map<std::string, std::string> mimeTypes;
		static const std::map<std::string, std::string> redirectCodes;

		static std::string	getActualTimeStringGMT();
		static void			checkStatus(std::string& statusCode, std::string& statusMessage);
		static void			modifyEpollEventClient(Epoll&, Client*, uint32_t);
		static void			modifyEpollEvent(Epoll&, Client*, uint32_t);

		static std::string decodeUrl(std::string url);
		static std::string generateSessionId(); //-- BONUS : cookies

		static void			setCloexec(int fd);

		static void			toLower(std::string& str);
};
