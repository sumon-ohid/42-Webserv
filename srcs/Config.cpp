//-- Written by : msumon

#include "Config.hpp"
#include <cstddef>
#include <algorithm>
#include <sstream>
#include <string>

// --> Config class
// --> read the config file and store the values in the vector
// --> ignore lines that start with # but need to improve this to ignore comments after the config value
// --> need to add error handling for missing config values
// --> validation can be improved
Config::Config()
{
    std::string configFile = "Default";
}

Config::Config(std::string configFile) : configFile(configFile)
{
    readConfig(configFile);
    if (validationCheck() == false)
        throw std::runtime_error ("The config file is invalid.");
    //-- removing ; from line after validation
    for (size_t i = 0; i < configVector.size(); i++)
    {
        configVector[i].erase(remove(configVector[i].begin(), configVector[i].end(), ';'), configVector[i].end()); 
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
void Config::readConfig(std::string configFile)
{
    std::ifstream file(configFile.c_str());
    std::string line;
    std::string newLine;

    if (file.is_open())
    {
        while (std::getline(file, line))
        {
            if (line.empty())
                continue;
            if (line.find("server") != std::string::npos)
            {
                size_t i = 0;
                while (line[i])
                {
                    if (line[i] == '{' || line[i] == '}' || line[i] == ';')
                    {
                        line.insert(i + 1, 1, '\n');
                        i++;
                    }
                    i++;
                }
            }
            std::istringstream iss(line);
            std::string splitLine;
            while (std::getline(iss, splitLine, '\n'))
            {
                if (splitLine.empty())
                    continue;
                newLine = removeLeadingSpaces(splitLine);
                if (newLine.empty())
                    continue;
                if (newLine.find("#") != std::string::npos)
                {
                    size_t pos = newLine.find("#");
                    newLine = newLine.erase(pos);
                    if (newLine.empty())
                        continue;
                }
                configVector.push_back(newLine);
            }
        }
        file.close();
    }
    else
    {
        throw std::runtime_error ("cannot open config file");
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
            throw std::runtime_error ("Missing ; or { } in the config file.");
        i++;
    }
    i = 0;
    bool bracketFlag = false;
    bool checker = false;
    int countOpen = 0;
    int countClose = 0;

    while (i < temp.size())
    {
        countOpen += std::count(temp[i].begin(), temp[i].end(), '{');
        countClose += std::count(temp[i].begin(), temp[i].end(), '}');

        std::string line = temp[i];
        if (line.find('{') != std::string::npos)
            checker = true;
        if (checker && line.find('}' ) != std::string::npos)
            bracketFlag = true;
        i++;
    }
    if (bracketFlag == false)
        return (false);
    if (countOpen != countClose)
        return (false);
    return (true);
}
