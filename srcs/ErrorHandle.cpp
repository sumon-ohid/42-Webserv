//-- Written by : msumon

#include "ErrorHandle.hpp"
#include "Client.hpp"
#include "Helper.hpp"

#include "ServerConfig.hpp"
#include <cstddef>
#include <iostream>

ErrorHandle::ErrorHandle()
{
    errorFile = "";
    errorStatusCode = "";
    errorMessage = "";
    pageTitle = "Error Page";
}

std::string ErrorHandle::modifyErrorPage()
{
    size_t pos = errorFile.rfind("/");
    std::string fileDirectory = errorFile.substr(0, pos + 1);

    //-- open the default error file
    std::ifstream defaultErrorFile(errorFile.c_str());

    //-- create a new error file, error code will be the file name
    std::string tempFileName = fileDirectory + errorStatusCode + ".html";
    newErrorFile = tempFileName;
    std::ofstream newErrorFile(tempFileName.c_str());

    if (defaultErrorFile.is_open() && newErrorFile.is_open())
    {
        std::string line;
        while (std::getline(defaultErrorFile, line))
        {
            if (line.find("0xvcqtr") != std::string::npos)
                line.replace(line.find("0xvcqtr"), 7, errorStatusCode);
            if (line.find("1xlssld") != std::string::npos)
                line.replace(line.find("1xlssld"), 7, errorMessage);
            if (line.find("2xpoqwq") != std::string::npos)
                line.replace(line.find("2xpoqwq"), 7, pageTitle);
            newErrorFile << line << std::endl;
        }
    }
    else
        throw std::runtime_error("No error file found in Config!!");

    newErrorFile.close();
    defaultErrorFile.close();

    // Re-open the new error file to read its contents
    std::ifstream readNewErrorFile(tempFileName.c_str());
    std::ostringstream buffer;
    buffer << readNewErrorFile.rdbuf();
    std::string body = buffer.str();
    readNewErrorFile.close();

    errorBody = body;
    return body;

    // -- Remove the files before quiting the program

    //std::remove(tempFileName.c_str());
}

void ErrorHandle::prepareErrorPage(Client *client, std::string statusCode)
{
    std::string errorPage = client->_server->_serverConfig.getErrorPage();
    std::map<std::string, std::string>::const_iterator it;
	it = Helper::statusCodes.find(statusCode);
	std::string statusMessage;
	if (it == Helper::statusCodes.end()) {
		statusCode = "500";
		statusMessage = "Internal Server Error";
	} else {
		statusMessage = it->second;
	}

    errorFile = errorPage;
    errorStatusCode = statusCode;
    errorMessage = statusMessage;
    pageTitle = statusCode + " " + statusMessage;
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
    // -- Remove the error file
    std::remove(newErrorFile.c_str());
}

void ErrorHandle::setErrorFile(std::string errorFile)
{
    this->errorFile = errorFile;
}

void ErrorHandle::setErrorStatusCode(std::string errorStatusCode)
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

std::string ErrorHandle::getErrorStatusCode()
{
    return errorStatusCode;
}

std::string ErrorHandle::getErrorMessage()
{
    return errorMessage;
}

std::string ErrorHandle::getNewErrorFile()
{
    return newErrorFile;
}
