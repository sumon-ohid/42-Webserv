#include "Config.hpp"
#include <cstddef>
#include <vector>

Config::Config()
{
    std::string configFile = "Default";
}

Config::Config(std::string configFile) : configFile(configFile)
{
    readConfig();
    cleanComments();
    if (validationCheck() == false)
    {
        std::cerr << "The config file is invalid." << std::endl;
        exit(1);
    }
}

Config::~Config()
{
}

void Config::setConfig(std::string line)
{
    configVector.push_back(line);
}

std::vector<std::string> Config::getConfig()
{
    return (configVector);
}

std::string Config::removeLeadingSpaces(std::string line)
{
    size_t pos = line.find_first_not_of(' ');
    if (pos == std::string::npos)
        return "";
    std::string newLine = line.substr(pos);
    return (newLine);
}

// --> read the config file and store the values in the map
// --> ignore lines that start with # but need to improve this to ignore comments after the config value
// --> need to add error handling for missing config values
void Config::readConfig()
{
    std::ifstream file(configFile);
    std::string line;
    std::string newLine;
    std::string key;
    std::string value;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;
            newLine = removeLeadingSpaces(line);
            configVector.push_back(newLine);
        }
        file.close();
    }
    else
    {
        std::cerr << "Error: cannot open config file" << std::endl;
        exit(1);
    }
}

void Config::cleanComments()
{
    std::vector<std::string> temp = configVector;
    configVector.clear();
    size_t count = 0;
    while (count < temp.size())
    {
        std::string line = temp[count];
        if (line[0] == '#')
            temp[count].erase();
        if (!temp[count].empty())
            configVector.push_back(temp[count]);
        count++;
    }
}

bool Config::validationCheck()
{
    size_t i = 0;
    std::vector<std::string> temp = configVector;

    while (i < temp.size())
    {
        std::string line = temp[i];
        size_t pos = line.find_last_not_of(' ');
        if (line[pos] != ';' && line[pos] != '{' && line[pos] != '}')
        {
            std::cerr << "Missing ; or { } in the config file." << std::endl;
            return (false);
        }
        i++;
    }
    i = 0;
    bool bracketFlag = false;
    bool checker = false;
    while (i < temp.size())
    {
        std::string line = temp[i];
        if (line.find('{'))
            checker = true;
        if (checker && line.find('}'))
            bracketFlag = true;
        i++;
    }
    if (bracketFlag == false)
        return (false);
    return (true);
}

// --> ServerConfig class
ServerConfig::ServerConfig() : LocationConfig()
{
}

ServerConfig::ServerConfig(std::string configFile) : LocationConfig(configFile)
{
    std::vector<std::string> configVector = Config::getConfig();
    size_t i = 0;
    
    while (i < configVector.size())
    {
        std::string line = configVector[i];
        
        if (line.find("server") != std::string::npos)
        {
            ServerConfig server;
            i++;
            while (i < configVector.size())
            {
                line = configVector[i];
                
                // Check for the end of the server block
                if (line.find('}') != std::string::npos)
                    break;
                
                // Process server directives
                if (line.find("listen") == 0)
                {
                    size_t pos = line.find(" ");
                    if (pos != std::string::npos)
                        server.listenPort = line.substr(pos + 1);
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
                else if (line.find("location") == 0)
                {
                    LocationConfig locationConfig(configFile);
                    
                    // Get location path
                    size_t pos = line.find(" ");
                    if (pos != std::string::npos)
                        locationConfig.setPath(line.substr(pos + 1));

                    // Process location block
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
                
                i++;
            }
            servers.push_back(server);
        }
        
        i++;
    }
}

void ServerConfig::displayConfig()
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        ServerConfig server = servers[i];
        std::cout << "Server: " << i + 1 << std::endl;
        std::cout << "Listen Port: " << server.listenPort << std::endl;
        std::cout << "Server Name: " << server.serverName << std::endl;
        std::cout << "Error Page: " << server.errorPage << std::endl;
        std::cout << "Locations: " << std::endl;
        
        for (size_t j = 0; j < server.locations.size(); j++)
        {
            LocationConfig location = server.locations[j];
            std::multimap<std::string, std::string > locationMap = location.getLocationMap();
            std::cout << "Path: " << location.getPath() << std::endl;
            std::cout << "Location Map: " << std::endl;
            std::multimap<std::string, std::string >::iterator it;
            for ( it = locationMap.begin(); it != locationMap.end(); ++it)
            {
                std::cout << it->first << " : " << it->second << std::endl;
            }
        }
        std::cout << std::endl;
    }
}

ServerConfig::~ServerConfig()
{
}

// --> LocationConfig class
LocationConfig::LocationConfig() : Config()
{
}

void LocationConfig::insertInMap(std::string key, std::string value)
{
    locationMap.insert(std::pair<std::string, std::string>(key, value));
}

LocationConfig::LocationConfig(std::string configFile) : Config(configFile)
{
}

void LocationConfig::setPath(std::string path)
{
    this->path = path;
}

std::string LocationConfig::getPath()
{
    return (path);
}

std::multimap<std::string, std::string> LocationConfig::getLocationMap()
{
    return (locationMap);
}

LocationConfig::~LocationConfig()
{
}