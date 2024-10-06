//-- Written by : msumon

#include "LocationConfig.hpp"

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
