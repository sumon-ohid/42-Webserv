#include "Config.hpp"
#include <cstddef>
#include <cstring>
#include <iterator>
#include <map>
#include <string>
#include <vector>

Config::Config(std::string configFile) : configFile(configFile)
{
    readConfig();
    cleanComments();
    if (validationCheck() == false)
    {
        std::cerr << "The config file is invalid." << std::endl;
        exit(1);
    }
    multiMapMaker();
}

Config::~Config()
{
}

void Config::setConfig(std::string key, std::string value)
{
    configMap.insert(std::pair<std::string, std::string>(key, value));
}

std::string Config::getConfig(std::string key)
{
    std::map<std::string, std::string>::iterator it = configMap.find(key);
    if (it != configMap.end())
        return (it->second);
    return ("");
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

// --> print the config values
void Config::printConfig()
{
    // size_t i = 0;
    // while (i < configVector.size())
    // {
    //     std::cout << configVector[i] << std::endl;
    //     i++;
    // }

    std::map<std::string, std::string>::iterator it = configMap.begin();
    while (it != configMap.end())
    {
        std::cout << it->first << " --- " << it->second << std::endl;
        it++;
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

void Config::multiMapMaker()
{
    std::vector<std::string> config = configVector;
    size_t i = 1;

    while (i < config.size())
    {
        std::string line = config[i];
        //std::cout << "--->" << line << std::endl;
        if (line.find("location") != std::string::npos || 
            line.find("{") != std::string::npos || 
            line.find("}") != std::string::npos)
        {
            i++;
            continue;
        }

        size_t pos = line.find(" ");
        if (pos != std::string::npos)
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            setConfig(key, value);
        }

        i++;
    }
}
