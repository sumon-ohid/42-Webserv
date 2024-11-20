#include "../includes/LocationFinder.hpp"
#include "../includes/Server.hpp"
#include "../includes/Helper.hpp"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <dirent.h>

LocationFinder::LocationFinder()
{
    _root = "";
    _index = "";
    _autoIndex = "";
    _redirect = "";
    _allowed_methods = "";
    _pathToServe = "";
    _locationPath = "";
    _defaultRoot = "./www";
    _clientMaxBodySize = "";

    _cgiFound = false;
    _autoIndexFound = false;
    _allowedMethodFound = false;
    _redirectFound = false;
    _clientBodySizeFound = false;
    _autoIndexMode = false;

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
    std::string root;
    for (size_t i = 0; i < locationsVector.size(); i++)
    {
        std::string tempPath = locationsVector[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        if (path == tempPath)
        {
            std::multimap<std::string, std::string> locationMap = locationsVector[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;

            for (it = locationMap.begin(); it != locationMap.end(); it++)
            {
                if (it->first == "root")
                    root = it->second;
            }
        }
    }
    std::string fullPath = root + path;

    struct stat pathStat;
    if (stat(fullPath.c_str(), &pathStat) == 0 && S_ISDIR(pathStat.st_mode))
        return true;
    return false;
}

bool LocationFinder::searchIndexHtml(const std::string &directory, std::string &foundPaths)
{
    DIR* dir = opendir(directory.c_str());
    if (dir == NULL)
    {
        std::cerr << BOLD RED << "Failed to open directory: " << directory << RESET << std::endl;
        return false;
    }

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL)
    {
        if (std::string(ent->d_name) == "index.html")
        {
            std::string path = directory + "/" + ent->d_name;
            struct stat st;
            if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            {
                foundPaths = path;
                closedir(dir);
                return true;
            }
            else if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                searchIndexHtml(path, foundPaths);
        }
    }
    closedir(dir);
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
    locationsVector = client->_request.back()._servConf->getLocations();
    if (locationsVector.empty()) {
        LocationConfig locConfig;
        locConfig.setPath("/");
        locConfig.insertInMap("root", _defaultRoot);
        locationsVector.push_back(locConfig);
    }

    //-- Remove the last slash from the path to avoid mismatch.
    if (path != "/" && path[path.size() - 1] == '/')
    {
        size_t pos = path.find_last_not_of("/");
        path = path.substr(0, pos + 1);
    }
    else if (path != "/" && path[path.size() - 1] != '/'
        && isDirectory(path) && client->_request.back().getMethodName() == "GET"
        && path.find("cgi-bin") == std::string::npos)
    {
        _redirect = "301 " + path + "/";
        _redirectFound = true;
        return true;
    }
    requestPath = path;

    for (size_t i = 0; i < locationsVector.size(); i++)
    {
        std::string tempPath = locationsVector[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        if (requestPath == tempPath)
        {
            std::multimap<std::string, std::string> locationMap = locationsVector[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;

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
                if (it->first == "client_max_body_size") {
                    _clientMaxBodySize = it->second;
                    _clientBodySizeFound = true;
                }
            }
            //-- IF location has no Index or Root, It will search for index,html
            //-- In the current directory, if inde.html is a dir
            //-- It will open it and look for index.html file.
            if (locationMap.find("root") == locationMap.end() && !_autoIndexFound && !_redirectFound)
            {
                std::string path = _defaultRoot + tempPath;
                searchIndexHtml(path, _pathToServe);
                return true;
            }

            if (locationMap.find("index") == locationMap.end() && !_autoIndexFound && !_redirectFound)
            {
                if (locationMap.find("root") != locationMap.end())
                    _defaultRoot = locationsVector[0].getLocationMap().find("root")->second;
                std::string path = _defaultRoot + tempPath;
                searchIndexHtml(path, _pathToServe);
                return true;
            }

            if (requestPath.find("cgi-bin") != std::string::npos)
                _cgiFound = true;

            _pathToServe = _root + tempPath + "/" + _index;
            _locationPath = tempPath;

            return true;
        }
    }
    //-- To check if cgi request with correct extention or not
    std::string extension;
    size_t pos = requestPath.rfind(".");
    if (pos != std::string::npos)
        extension = requestPath.substr(pos);
    
    _root = _defaultRoot;
    if (locationsVector[0].getLocationMap().find("root") != locationsVector[0].getLocationMap().end())
        _root = locationsVector[0].getLocationMap().find("root")->second;
    
    _pathToServe = _root + _locationPath + path;

    if (isDirectory(_pathToServe))
    {   
        if (!_autoIndexMode && searchIndexHtml(_pathToServe, _pathToServe))
            return true;
        else
        {
            _pathToServe = _root + _locationPath + path + "/";
            _autoIndexFound = true;
            _autoIndex = "on";
            return true;
        }
    }
    else if (requestPath.find("cgi-bin") != std::string::npos && requestPath.size() > 9 && !extension.empty() && Helper::executableMap.find(extension) != Helper::executableMap.end())
    {
        _pathToServe = _root + requestPath;
        _cgiFound = true;
        return true;
    }
    // else if (!isDirectory(_pathToServe) && !_autoIndexFound)
    //     _autoIndexMode = false;
    //std::cout << BOLD RED << "PATH TO SERVE " << _pathToServe << RESET << std::endl;
    return false;
}
