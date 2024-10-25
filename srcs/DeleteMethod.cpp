#include "../includes/DeleteMethod.hpp"
#include "../includes/Request.hpp"
#include <cerrno>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

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
	// _pathToDelete = root + request.getMethodPath();
	deleteObject();
	(void)socketFd;
	(void)client;
	(void)request;
}

void	DeleteMethod::deleteObject()
{
	struct stat statInfo;
	if (stat(_pathToDelete.c_str(), &statInfo) == -1)
		checkStatError();
	if (S_ISREG(statInfo.st_mode) || S_ISLNK(statInfo.st_mode))
	{
		if (remove(_pathToDelete.c_str()) == -1)
			checkStatError();
		throw std::runtime_error("204");
	}
	else if (S_ISDIR(statInfo.st_mode))
	{
		if (rmdir(_pathToDelete.c_str()) == -1)
			checkStatError();
		throw std::runtime_error("204");
	}
	else
		throw std::runtime_error("415");

}

void	DeleteMethod::checkStatError()
{
	switch (errno)
	{
		case ENOENT:
			throw std::runtime_error("404");
		case EACCES:
			throw std::runtime_error("403");
		case ENOTEMPTY:
			throw std::runtime_error("409");
		default:
			throw std::runtime_error("500");
	}
}














Method*	DeleteMethod::clone()
{
	return new DeleteMethod(*this);
}