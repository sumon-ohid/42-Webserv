#ifndef METHOD_HPP
#define METHOD_HPP

class Request;
class Client;

#include <string>

#define ARRAY_SIZE 3

class Method {
	protected:
		std::string	_name;
		std::string _path;
		std::string _protocol;
		std::string _mimeType;

	public:
		static const std::string	_methodArray[ARRAY_SIZE];

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

		virtual void	executeMethod(int socketFd, Client *client, Request &request) = 0;
		virtual Method*	clone() = 0;

};

#endif
