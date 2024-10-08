//-- Written by : msumon

#include "ServerConfig.hpp"
#include <cstddef>
#include <vector>

ServerConfig::ServerConfig() : LocationConfig()
{
}

void ServerConfig::locationBlock(std::string line, size_t &i, std::vector<std::string> configVector, ServerConfig &server, std::string configFile)
{
    LocationConfig locationConfig(configFile);
    size_t pos = line.find(" ");
    if (pos != std::string::npos)
        locationConfig.setPath(line.substr(pos + 1));
    i++;
    while (i < configVector.size())
    {
        line = configVector[i];
        if (line.find('}') != std::string::npos)
            break;
        pos = line.find(' ');
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            locationConfig.insertInMap(key, value);
        }
        i++;
    }
    server.locations.push_back(locationConfig);
}

void ServerConfig::serverBlock(std::string line, size_t &i, std::vector<std::string> configVector, ServerConfig &server, std::string configFile)
{
    while (i < configVector.size())
    {
        line = configVector[i];
        if (line.find('}') != std::string::npos)
            break;
        // Process server directives
        if (line.find("listen") == 0)
        {
            size_t pos = line.find(" ");
            if (pos != std::string::npos)
            {
                server.listenPort = line.substr(pos + 1);
                server.makePortVector();
            }
        }
        else if (line.find("server_name") == 0)
        {
            size_t pos = line.find(" ");
            if (pos != std::string::npos)
                server.serverName = line.substr(pos + 1);
        }
        else if (line.find("error_page") == 0)
        {
            size_t pos = line.find(" ");
            if (pos != std::string::npos)
                server.errorPage = line.substr(pos + 1);
        }
        else if (line.find("cgi-bin") == 0)
        {
            size_t pos = line.find(" ");
            if (pos != std::string::npos)
                server.cgiFile = line.substr(pos + 1);
        }
        else if (line.find("location") == 0)
            locationBlock(line, i, configVector, server, configFile);
        i++;
    }
}

//-- Constructor with parameter
ServerConfig::ServerConfig(std::string configFile) : LocationConfig(configFile)
{
    this->_configFile = configFile;
    std::vector<std::string> configVector = Config::getConfig();
    size_t i = 0;
    
    while (i < configVector.size())
    {
        std::string line = configVector[i];
        if (line.find("server") != std::string::npos)
        {
            ServerConfig server;
            i++;
            serverBlock(line, i, configVector, server, configFile);
            servers.push_back(server);
        }
        i++;
    }
}

//--- > To Print the config after parsing
void ServerConfig::displayConfig()
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        ServerConfig server = servers[i];
        std::cout << "Server: " << i + 1 << std::endl;
        std::cout << "Listen Port: " << server.listenPort << std::endl;
        std::cout << "Server Name: " << server.serverName << std::endl;
        std::cout << "Error Page: " << server.errorPage << std::endl;
        std::cout << "CGI File: " << server.cgiFile << std::endl;
        std::cout << "Locations: " << std::endl;
        
        for (size_t k = 0; k < server.getListenPorts().size(); k++)
        {
            std::cout << "Multiple Port " << server.getListenPorts()[k] << std::endl;
        }

        for (size_t j = 0; j < server.locations.size(); j++)
        {
            LocationConfig location = server.locations[j];
            std::multimap<std::string, std::string > locationMap = location.getLocationMap();
            std::cout << "Path: " << location.getPath() << std::endl;
            std::multimap<std::string, std::string >::iterator it;
            for ( it = locationMap.begin(); it != locationMap.end(); ++it)
            {
                std::cout << it->first << " : " << it->second << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

void ServerConfig::makePortVector()
{
    std::vector<int> tempPorts;
    std::string ports = getListenPort();

    while (!ports.empty())
    {
        size_t pos = ports.find(" ");
        std::string temp = ports.substr(0, pos);
        int port = atoi(temp.c_str());
        tempPorts.push_back(port);
        if (pos == std::string::npos)
            break;
        ports.erase(0, pos + 1);
    }
    listenPorts = tempPorts;
}

//--- > Get functions
std::string ServerConfig::getListenPort()
{
    return (listenPort);
}

std::string ServerConfig::getServerName()
{
    return (serverName);
}

std::string ServerConfig::getErrorPage()
{
    return (errorPage);
}

std::string ServerConfig::getCgiFile()
{
    return (cgiFile);
}

std::vector<LocationConfig> ServerConfig::getLocations()
{
    return (locations);
}

std::vector<ServerConfig> ServerConfig::getServers()
{
    return (servers);
}

std::vector<int> ServerConfig::getListenPorts()
{
    return (listenPorts);
}

ServerConfig::~ServerConfig()
{
}
