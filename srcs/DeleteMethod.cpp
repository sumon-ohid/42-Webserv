#include "../includes/DeleteMethod.hpp"
#include "../includes/Request.hpp"
#include "../includes/LocationFinder.hpp"
#include "../includes/Response.hpp"

#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <iostream>

DeleteMethod::DeleteMethod() {setName("DELETE");}
DeleteMethod::DeleteMethod(const DeleteMethod& orig) : Method(orig), _pathToDelete(orig._pathToDelete) {}
DeleteMethod::~DeleteMethod() {}
DeleteMethod&	DeleteMethod::operator=(const DeleteMethod &rhs) 
{
	if (this != &rhs)
	{
		Method::operator=(rhs);
		_pathToDelete = rhs._pathToDelete;
	}
	return (*this);
}

void	DeleteMethod::executeMethod(int socketFd, Client* client, Request& request)
{
	LocationFinder locationFinder;
	locationFinder.locationMatch(client, request.getMethodPath() ,socketFd);
	std::string root = locationFinder._root;
	size_t pos = root.find_last_of("/");
	root = root.substr(0, pos);

	_pathToDelete = root + request.getMethodPath();

	deleteObject();

	//-- headerAndBody is always 200, but we need for different codes as well.
	std::string body = "<html><body><h1>File Deleted Successfully!</h1></body></html>";
	Response::headerAndBody(socketFd, request, body);
}

void	DeleteMethod::deleteObject()
{
	struct stat statInfo;
	if (stat(_pathToDelete.c_str(), &statInfo) == -1)
		checkStatError();
	if (S_ISREG(statInfo.st_mode) || S_ISLNK(statInfo.st_mode))
	{
		if (std::remove(_pathToDelete.c_str()) == -1)
			checkStatError();
		std::cout << BOLD RED << "FILE REMOVED : " << _pathToDelete << RESET << " 📁🗑️" << std::endl;
		_statusCode = 204;
	}
	else if (S_ISDIR(statInfo.st_mode))
	{
		if (std::remove(_pathToDelete.c_str()) == -1)
			checkStatError();
		std::cout << BOLD RED << "FILE REMOVED : " << _pathToDelete << RESET << " 📁🗑️" << std::endl;
		_statusCode = 204;
	}
	else
		_statusCode = 415;

}

void	DeleteMethod::checkStatError()
{
	switch (errno)
	{
		case ENOENT:
			_statusCode = 404;
		case EACCES:
			_statusCode = 403;
		case ENOTEMPTY:
			_statusCode = 409;
		default:
			_statusCode = 500;
	}
}














Method*	DeleteMethod::clone()
{
	return new DeleteMethod(*this);
}