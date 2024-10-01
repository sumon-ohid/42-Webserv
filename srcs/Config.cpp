#include "Config.hpp"

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
