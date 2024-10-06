//-- Written by : msumon

#pragma once

#include "ServerConfig.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>


class ErrorHandle : public ServerConfig
{
    private:
        std::string errorFile;
        size_t errorStatusCode;
        std::string errorMessage;

        std::vector<ErrorHandle> errorVector;

    public:
        ErrorHandle();
        ErrorHandle(std::string configFile);
        ~ErrorHandle();

        void setErrorFile(std::string errorFile);
        void setErrorStatusCode(size_t errorStatusCode);
        void setErrorMessage(std::string errorMessage);

        std::string getErrorFile();
        size_t getErrorStatusCode();
        std::string getErrorMessage();
        std::vector<ErrorHandle> getErrorVector();
        
        void displayError();
        void modifyErrorPage();
        
        template <typename T>
        std::string to_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
