#pragma once

#include "Client.hpp"

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

        bool _cgiFound;
        bool _autoIndexFound;
        bool _allowedMethodFound;
        bool _redirectFound;

        LocationFinder();
        LocationFinder& operator=(const LocationFinder& other);
        ~LocationFinder();

        bool locationMatch(Client *client, std::string, int socketFd);
        void handleRedirection(std::string &redirectUrl, Request &request);
        bool isDirectory(const std::string &path);
};
