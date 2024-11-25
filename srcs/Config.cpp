//-- Written by : msumon

#include "../includes/ServerConfig.hpp"

#include <algorithm>
#include <cstddef>
#include <sstream>
#include <iostream>
#include <fstream>

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
    if (validationCheck() == false || syntaxCheck() == false)
        throw std::runtime_error(BOLD + configFile + RED + " [ KO ] " + RESET);
    for (size_t i = 0; i < configVector.size(); i++)
        configVector[i].erase(remove(configVector[i].begin(), configVector[i].end(), ';'), configVector[i].end());
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
        throw std::runtime_error (BOLD RED "Unable to open Config file [ " + configFile + " ]" RESET);
    }
}

bool Config::validationCheck()
{
    size_t i = 0;
    std::vector<std::string> temp = configVector;

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

static bool semiColonCheck(std::string line, std::string keyword)
{
    if (line.find(keyword) == std::string::npos)
    {
        std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
        return (false);
    }
    std::string temp;
    size_t pos = line.find(" ");
    if (pos != std::string::npos)
        temp = line.substr(0, pos);
    if (temp != keyword)
    {
        std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
        return (false);
    }

    pos = line.find_first_of(" ;}");
    if (pos == std::string::npos)
    {
        std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
        return (false);
    }
    pos = std::count(line.begin(), line.end(), ';');
    if (pos != 1)
    {
        std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
        return (false);
    }
    return (true);
}

bool Config::syntaxCheck()
{
    size_t i = 0;
    std::string keyword;
    std::vector<std::string> temp = configVector;

    while (i < temp.size())
    {
        std::string line = temp[i];
        size_t pos = line.find_last_not_of(" ");
        line = line.substr(0, pos + 1);
        if (line.find("server") != std::string::npos)
        {
            if (line.find("server_name") != std::string::npos)
            {
                if (semiColonCheck(line, "server_name") == false)
                    return (false);
            }
            else
            {
                line.erase(remove(line.begin(), line.end(), '{'), line.end());
                line.erase(remove(line.begin(), line.end(), ' '), line.end());
                if (line != "server")
                {
                    std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
                    return false;
                }
            }
        }
        else if (line.find("listen") != std::string::npos)
        {
            if (semiColonCheck(line, "listen") == false)
                return (false);
        }
        else if (line.find("error_page") != std::string::npos)
        {
            if (semiColonCheck(line, "error_page") == false)
                return (false);
        }
        else if (line.find("client_max_body_size") != std::string::npos)
        {
            if (semiColonCheck(line, "client_max_body_size") == false)
                return (false);
        }
        else if (line.find("set_timeout") != std::string::npos)
        {
            if (semiColonCheck(line, "set_timeout") == false)
                return (false);
        }
        else if (line.find("location") != std::string::npos)
        {
            std::string temp;
            size_t pos = line.find(" ");
            if (pos != std::string::npos)
                temp = line.substr(0, pos);
            if (temp != "location")
                return (false);

            line.erase(remove(line.begin(), line.end(), '{'), line.end());
            pos = line.find(" ");
            if (pos != std::string::npos)
                temp = line.substr(pos + 1);
            //-- check if the location path not empty
            if (temp.empty())
                return (false);
            pos = line.find("/");
            if (pos != std::string::npos)
                temp = line.substr(0, pos);
            temp.erase(remove(temp.begin(), temp.end(), ' '), temp.end());
            if (temp != "location")
                return (false);
        }
        else if (line.find("allowed_methods") != std::string::npos)
        {
            if (semiColonCheck(line, "allowed_methods") == false)
                return (false);
        }
        else if (line.find("root") != std::string::npos)
        {
            if (semiColonCheck(line, "root") == false)
                return (false);
        }
        else if (line.find("autoindex") != std::string::npos)
        {
            if (semiColonCheck(line, "autoindex") == false)
                return (false);
        }
        else if (line.find("index") != std::string::npos)
        {
            if (semiColonCheck(line, "index") == false)
                return (false);
        }
        else if (line.find("cgi_bin") != std::string::npos)
        {
            if (semiColonCheck(line, "cgi_bin") == false)
                return (false);
        }
        else if (line.find("return") != std::string::npos)
        {
            if (semiColonCheck(line, "return") == false)
                return (false);
        }
        else if (line == "}" || line == "{")
            i++;
        else
        {
            std::cerr << std::endl << BOLD RED << "LINE : " << line << "  [ NOT VALID ]" << RESET << std::endl;
            return (false);
        }
        i++;
    }
    return (true);
}
