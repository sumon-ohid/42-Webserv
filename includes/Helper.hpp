#pragma once

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

		static std::string getActualTimeStringGMT();
};
