#pragma once

#include "Client.hpp"
#include "LocationConfig.hpp"

#include <string>

class LocationFinder
{
	private:
        std::vector<LocationConfig> locationsVector;

	public:
        int socketFd;
        std::string _root;
        std::string _index;
        std::string _autoIndex;
        std::string _redirect;
        std::string _allowed_methods;
        std::string _pathToServe;
        std::string _locationPath;
        std::string _defaultRoot;
        std::string _clientMaxBodySize;

        bool _cgiFound;
        bool _autoIndexFound;
        bool _allowedMethodFound;
        bool _redirectFound;
        bool _clientBodySizeFound;

        LocationFinder();
        LocationFinder& operator=(const LocationFinder& other);
        ~LocationFinder();

        bool locationMatch(Client *client, std::string, int socketFd);
        bool isDirectory(const std::string &path);
        bool searchIndexHtml(const std::string &directory, std::string &foundPaths);
};
