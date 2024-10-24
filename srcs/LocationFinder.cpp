#include "../includes/LocationFinder.hpp"
#include "../includes/Server.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <sys/stat.h>

LocationFinder::LocationFinder()
{
    _root = "";
    _index = "";
    _autoIndex = "";
    _redirect = "";
    _allowed_methods = "";
    _pathToServe = "";
    _locationPath = "";

    _cgiFound = false;
    _autoIndexFound = false;
    _allowedMethodFound = false;
    _redirectFound = false;
    
    locationsVector.clear();
}

LocationFinder&	LocationFinder::operator=(const LocationFinder& other)
{
	if (this == &other)
		return *this;
	return *this;
}

LocationFinder::~LocationFinder() {}

//-- Check if the path is a directory.
bool LocationFinder::isDirectory(const std::string &path)
{
    struct stat pathStat;
    if (stat(path.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
        return true;
    return false;
}

//-- RequestPath should be the location of the request.
//-- If the request path matches with the location path,
//-- then get the root and index and other values.
//-- It will return false if location not found.
bool LocationFinder::locationMatch(Client *client, std::string path, int _socketFd)
{
    std::string requestPath;
    socketFd = _socketFd;
 
    //-- Remove the last slash from the path to avoid mismatch.
    if (path != "/" && path[path.size() - 1] == '/')
    {
        size_t pos = path.find_last_not_of("/");
        path = path.substr(0, pos + 1);
    }
    requestPath = path;

    //std::cout << BOLD BLUE << "PATH " << path << RESET << std::endl;
    //std::cout << BOLD BLUE << "REQUEST PATH " << requestPath << RESET << std::endl;

    locationsVector = client->_server->_serverConfig.getLocations();
    for (size_t i = 0; i < locationsVector.size(); i++)
    {
        std::string tempPath = locationsVector[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        if (requestPath == tempPath)
        {
            std::multimap<std::string, std::string> locationMap = locationsVector[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;
            
            if (requestPath.find("cgi-bin") != std::string::npos) {
                _cgiFound = true;
                _root = locationMap.find("root")->second;
                _index = locationMap.find("index")->second;
                _allowed_methods = locationMap.find("allowed_methods")->second;
                return true;
            }

            for (it = locationMap.begin(); it != locationMap.end(); it++)
            {
                if (it->first == "root")
                    _root = it->second;
                if (it->first == "index")
                    _index = it->second;
                if (it->first == "return") {
                    _redirect = it->second;
                    _redirectFound = true;
                }
                if (it->first == "autoindex") {
                    _autoIndex = it->second;
                    _autoIndexFound = true;
                }
                if (it->first == "allowed_methods") {
                    _allowed_methods = it->second;
                    _allowedMethodFound = true;
                }
            }
            _pathToServe = _root + tempPath + "/" + _index;
            _locationPath = tempPath;

            //std::cout << BOLD BLUE << "PATH TO SERVE " << _pathToServe << RESET << std::endl;
            
            return true;
        }
    }
    _root = locationsVector[0].getLocationMap().find("root")->second;
    _pathToServe = _root + _locationPath + path;
    //std::cout << BOLD RED << "PATH TO SERVE " << _pathToServe << RESET << std::endl;
    return false;
}
