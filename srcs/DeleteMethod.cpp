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
    locationFinder.locationMatch(client, request.getMethodPath(), socketFd);

	if (locationFinder._cgiFound)
	{
		request._response->error(request, "405", client);
		return;
    }

    //-- Check if the allowed methods include DELETE
    if (locationFinder._allowedMethodFound)
    {
        if (locationFinder._allowed_methods.find("DELETE") == std::string::npos)
        {
            request._response->error(request, "405", client);
            return;
        }
    }

	std::string root = locationFinder._root;
	size_t pos = root.find_last_of("/");
	root = root.substr(0, pos);
	_pathToDelete = root + request.getMethodPath();

	deleteObject();

	if (_statusCode == "204")
	{
		std::string body =  " <html><head> <title>File uploaded</title>"
        "  <style> body { display: flex; justify-content: center;"
        "  align-items: center; height: 100vh; font-family: Arial, sans-serif;"
        "  font-size: 1em; font-weight: bold; font-style: italic; } </style>"
        "  </head> <body> <h1>File Deleted successfully!</h1>"
        "  </body> </html>";

		request._response->createHeaderAndBodyString(request, body, "204", client);
	}
	else
		request._response->error(request, _statusCode, client);
}

void	DeleteMethod::deleteObject()
{
	struct stat statInfo;
	if (stat(_pathToDelete.c_str(), &statInfo) == -1)
	{
		checkStatError();
		return;
	}
	//-- Check if the file has write permissions
    if (access(_pathToDelete.c_str(), W_OK) == -1)
    {
        _statusCode = "403";
        std::cerr << BOLD RED << "Permission denied: " << _pathToDelete << RESET << std::endl;
        return;
    }
	if (S_ISREG(statInfo.st_mode) || S_ISLNK(statInfo.st_mode))
	{
		if (std::remove(_pathToDelete.c_str()) == -1)
		{
			checkStatError();
			return;
		}
		std::cout << BOLD RED << "FILE REMOVED : " << _pathToDelete << RESET << " 📁🗑️" << std::endl;
		_statusCode = "204";
	}
	else if (S_ISDIR(statInfo.st_mode))
	{
		if (std::remove(_pathToDelete.c_str()) == -1)
		{
			checkStatError();
			return;
		}
		std::cout << BOLD RED << "FILE REMOVED : " << _pathToDelete << RESET << " 📁🗑️" << std::endl;
		_statusCode = "204";
	}
	else
		_statusCode = "415";

}

void	DeleteMethod::checkStatError()
{
	switch (errno)
{
		case ENOENT:
		{
			_statusCode = "404";
			break;
		}
		case EACCES:
		{
			_statusCode = "403";
			break;
		}
		case ENOTEMPTY:
		{
			_statusCode = "409";
			break;
		}
		default:
		{
			_statusCode = "500";
			break;
		}
	}
}

Method*	DeleteMethod::clone()
{
	return new DeleteMethod(*this);
}
