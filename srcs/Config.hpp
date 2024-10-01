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
        std::multimap<std::string, std::string> configMap;
        std::vector<std::string> configVector;
        std::string configFile;

    protected:
        void setConfig(std::string key, std::string value);

    public:
        Config(std::string configFile);
        ~Config();
        std::string removeLeadingSpaces(std::string line);
        std::string getConfig(std::string key);
        void cleanComments();
        void readConfig();
        void printConfig();
        bool validationCheck();
        void multiMapMaker();
};
