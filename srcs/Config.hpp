#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

// --> idea is to have a class that will read the config file and store the values in a multimap
// --> multimap key will be the config name and the value will be the value of the config
// --> the class will have a function that will return the value of a config by passing the key
class Config
{
    private:
        std::vector<std::string> configVector;
        std::string configFile;
        
    public:
        Config();
        Config(std::string configFile);
        ~Config();

        void setConfig(std::string line);
        std::vector<std::string> getConfig();
        std::string removeLeadingSpaces(std::string line);
        void cleanComments();
        void readConfig();
        bool validationCheck();
        void multiMapMaker();
};

//--> TODO- remove mltiple space after "server" and "location" and before "{"

class LocationConfig : public Config
{
    private:
        std::string path;
        std::multimap<std::string, std::string > locationMap;

    public:
        LocationConfig();
        LocationConfig(std::string configFile);
        ~LocationConfig();

        void insertInMap(std::string key, std::string value);
        void setPath(std::string path);
        std::string getPath();
        std::multimap<std::string, std::string > getLocationMap();
};

class ServerConfig : public LocationConfig
{
    private:
        std::string listenPort;
        std::string serverName;
        std::string errorPage;
        std::vector<LocationConfig> locations;
        std::vector<ServerConfig> servers;

    public:
        ServerConfig();
        ServerConfig(std::string configFile);
        ~ServerConfig();

        void displayConfig();
};
