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
		static const std::map<std::string, std::string> statusCodes; //BP maybe move from here?

		static std::string getActualTimeString();
};
