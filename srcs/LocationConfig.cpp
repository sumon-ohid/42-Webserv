//-- Written by : msumon

#include "../includes/LocationConfig.hpp"

LocationConfig::LocationConfig() : Config()
{
}

bool	LocationConfig::operator==(const LocationConfig& other) const
{
    return (path == other.path &&
            locationMap == other.locationMap);
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
