//-- Written by : msumon

#pragma once

#include "ServerConfig.hpp"
#include "Client.hpp"
#include "Response.hpp"
#include "Server.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

class Client;
class ServerConfig;
class Server;

class ErrorHandle
{
    private:
        std::string errorFile;
        std::string errorStatusCode;
        std::string errorMessage;
        std::string pageTitle;
        std::string errorBody;
        std::string newErrorFile;

        std::vector<ErrorHandle> errorVector;

    public:
        ErrorHandle();
        ~ErrorHandle();

        void setErrorFile(std::string errorFile);
        void setErrorStatusCode(std::string errorStatusCode);
        void setErrorMessage(std::string errorMessage);

        std::string getErrorFile();
        std::string getErrorStatusCode();
        std::string getErrorMessage();
        std::vector<ErrorHandle> getErrorVector();
        std::string getNewErrorFile();
        
        void displayError();
        void prepareErrorPage(Client *client, std::string statusCode);
        std::string modifyErrorPage();
        
        template <typename T>
        std::string to_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
