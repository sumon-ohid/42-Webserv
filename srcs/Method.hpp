#ifndef METHOD_HPP
#define METHOD_HPP

#include <string>

#define ARRAY_SIZE 4

class Method {
	private:
		static const std::string	_mArray[ARRAY_SIZE];
		std::string	_name;

		Method();

	public:
		Method(std::string name);
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

};

#endif