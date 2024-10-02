#ifndef METHOD_HPP
#define METHOD_HPP

#include <string>
#include <vector>

#define ARRAY_SIZE 5

class Method {
	private:
		static const std::string	_mArray[ARRAY_SIZE];
		std::string	_name;
		std::string _path;
		std::string _protocol;

	public:
		Method();
		Method(std::string name, const std::string path, const std::string protocol);
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

		std::string getName() const;
		std::string getPath() const;
		std::string getProtocol() const;
		void	checkFirstLine(std::vector<char> line);

};

#endif