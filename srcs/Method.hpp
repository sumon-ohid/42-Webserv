#ifndef METHOD_HPP
#define METHOD_HPP

class Request;

#include <string>
#include <map>

#define ARRAY_SIZE 5

class Method {
	protected:
		std::string	_name;
		std::string _path;
		std::string _protocol;
		std::string _mimeType;

	public:
		static const std::string	_methodArray[ARRAY_SIZE];
		static const std::map<std::string, std::string> mimeTypes;

		Method();
		Method(const Method& other);
		Method&	operator=(const Method& other);
		bool	operator==(const Method& other) const;
		virtual ~Method();

		std::string getName() const;
		std::string getPath() const;
		std::string getProtocol() const;
		std::string getMimeType() const;

		void	setName(std::string name);
		void	setPath(std::string path);
		void	setProtocol(std::string protocol);
		void	setMimeType(std::string& path);

		virtual void	executeMethod(int socketFd, Request& request) = 0;

};

#endif
