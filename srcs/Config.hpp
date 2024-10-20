//-- Written by : msumon

#pragma once

#include <string>
#include <vector>

// --> Idea is to read the config file, remove comments, remove leading spaces
// --> Check the validation and syntax of the config file.
// --> Also remove the semi colon from the end of the line.
// --> Store the config values in a vector which will be used by the serverConfig class
class Config
{
    private:
        std::vector<std::string> configVector;
        std::string configFile;
        
    public:
        Config();
        Config(std::string configFile);
        ~Config();

        // --> set the config file
        void setConfig(std::string line);
        // --> get the config file vector
        std::vector<std::string> getConfig();
        // --> Remove leading spaces from a string
        std::string removeLeadingSpaces(std::string line);
        // --> Read the config file and store the values in the vector
        // --> It also removes the comments from the config file
        void readConfig(std::string configFile);
        // --> Check the validation of the config file
        // --> This checks brackets if closed properly
        bool validationCheck();
        // --> Check the syntax of the config file
        // --> This checks if the config values are correct
        // --> Check if the config values are in the correct format
        // --> Check if there is any invalid directive
        // --> Check for semi colon at the end of the line
        bool syntaxCheck();
};
