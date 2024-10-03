#ifndef METHOD_HPP
#define METHOD_HPP

#include <string>

#define ARRAY_SIZE 5

class Method {
	private:
		std::string	_name;
		std::string _path;
		std::string _protocol;

	public:
		static const std::string	_mArray[ARRAY_SIZE];

		Method();
		Method(const Method& other);
		Method& operator=(const Method& other);
		~Method();

		std::string getName() const;
		std::string getPath() const;
		std::string getProtocol() const;

		void	setName(std::string name);
		void	setPath(std::string path);
		void	setProtocol(std::string protocol);

};

#endif
