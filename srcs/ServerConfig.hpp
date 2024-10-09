//-- Written by : msumon

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include "LocationConfig.hpp"

// --> If you want to access the config values, initialize the ServerConfig class
// --> The ServerConfig class will have the server directives
// --> use the get functions to access the values

class ServerConfig : public LocationConfig
{
    private:
        std::string listenPort;
        std::string serverName; // BP: can't there be more than one servername (=Hosts)
        std::string errorPage;
        std::string cgiFile;
        std::vector<LocationConfig> locations;
        std::vector<ServerConfig> servers;
        std::vector<int> listenPorts;
        std::string _configFile;

    public:
        ServerConfig();
        ServerConfig(std::string configFile);
        ~ServerConfig();

        void displayConfig();
        std::string getListenPort();
        std::string getServerName();
        std::string getErrorPage();
        std::string getCgiFile();
        std::vector<LocationConfig> getLocations();
        std::vector<ServerConfig> getServers();

        void makePortVector();
        std::vector<int> getListenPorts();
        void serverBlock(std::string line, size_t &i, std::vector<std::string> configVector, ServerConfig &server, std::string configFile);
        void locationBlock(std::string line, size_t &i, std::vector<std::string> configVector, ServerConfig &server, std::string configFile);
};
