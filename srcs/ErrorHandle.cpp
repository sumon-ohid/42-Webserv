//-- Written by : msumon

#include "ErrorHandle.hpp"
#include "ServerConfig.hpp"

ErrorHandle::ErrorHandle()
{
    errorFile = "";
    errorStatusCode = 0;
    errorMessage = "";
}

void ErrorHandle::modifyErrorPage()
{
    // Modify the error page
    std::vector<ErrorHandle> errorVector = getErrorVector();
    for (size_t i = 0; i < errorVector.size(); i++)
    {
        ErrorHandle& error = errorVector[i];
        std::string errorFile = error.getErrorFile();
        std::string errorStatusCode = to_string(error.getErrorStatusCode());
        std::string errorMessage = error.getErrorMessage();
        std::ifstream file(errorFile.c_str());
        std::string tempFileName = errorStatusCode + ".html";
        std::ofstream tempFile(tempFileName.c_str());
        if (file.is_open() && tempFile.is_open())
        {
            std::string line;
            while (std::getline(file, line))
            {
                if (line.find("status_code") != std::string::npos)
                {
                    line.replace(line.find("status_code"), 11, errorStatusCode);
                }
                if (line.find("message") != std::string::npos)
                {
                    line.replace(line.find("message"), 7, errorMessage);
                }
                tempFile << line << std::endl;
            }
        }
        else  
        {
            throw std::runtime_error("Error opening errorPage file");
        }
        file.close();
        tempFile.close();

        // -- Remove the files before quiting the program

        //std::remove(errorFile.c_str());
        //std::rename(tempFileName.c_str(), errorFile.c_str());
        //std::remove(fileName.c_str());
    }
}   

ErrorHandle::ErrorHandle(std::string configFile) : ServerConfig(configFile)
{
    std::vector<ServerConfig> servers = getServers();
    for (size_t i = 0; i < servers.size(); i++)
    {
        ServerConfig server = servers[i];
        ErrorHandle newError;
        std::string errorPage = server.getErrorPage();
        if (!errorPage.empty())
            newError.setErrorFile(server.getErrorPage());
        newError.setErrorStatusCode(400);
        newError.setErrorMessage("Bad Request");
        errorVector.push_back(newError);
    }
    modifyErrorPage();
}

std::vector<ErrorHandle> ErrorHandle::getErrorVector()
{
    return errorVector;
}

void ErrorHandle::displayError()
{
    for (size_t i = 0; i < errorVector.size(); i++)
    {
        ErrorHandle& error = errorVector[i];
        std::cout << error.getErrorFile() << std::endl;
        std::cout << error.getErrorStatusCode() << std::endl;
        std::cout << error.getErrorMessage() << std::endl;
    }
}

ErrorHandle::~ErrorHandle()
{
}

void ErrorHandle::setErrorFile(std::string errorFile)
{
    this->errorFile = errorFile;
}

void ErrorHandle::setErrorStatusCode(size_t errorStatusCode)
{
    this->errorStatusCode = errorStatusCode;
}

void ErrorHandle::setErrorMessage(std::string errorMessage)
{
    this->errorMessage = errorMessage;
}

std::string ErrorHandle::getErrorFile()
{
    size_t pos = errorFile.find(";");
    errorFile = errorFile.substr(0, pos);
    return errorFile;
}

size_t ErrorHandle::getErrorStatusCode()
{
    return errorStatusCode;
}

std::string ErrorHandle::getErrorMessage()
{
    return errorMessage;
}
