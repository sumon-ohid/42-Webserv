
#include "../includes/PostMethod.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"
#include "../includes/Client.hpp"
#include "../includes/LocationFinder.hpp"
#include "../includes/HandleCgi.hpp"
#include "../includes/GetMethod.hpp"

#include <string>
#include <fstream>
#include <iomanip>
#include <cstdlib>

PostMethod::PostMethod() : socketFd(-1)
{
    saveDir = "";
    fileName = "";
    fileBody = "";
    root = "";
    locationPath = "";
}

PostMethod::PostMethod(const PostMethod& other) : Method(other), socketFd(other.socketFd) {}

PostMethod& PostMethod::operator=(const PostMethod& other)
{
    if (this == &other)
        return (*this);
    Method::operator=(other);
    return (*this);
}

PostMethod::~PostMethod() {}

//-- Execute the POST method
void PostMethod::executeMethod(int socketFd, Client *client, Request &request)
{
    std::string body = request._requestBody;
    std::string requestPath = request.getMethodPath();

    if (requestPath.find("/cgi-bin") != std::string::npos)
    {
        try
        {
            client->_cgi = HandleCgi(requestPath, socketFd, *client, request);
			// HandleCgi cgi(requestPath, socketFd, *client, request);
            std::cout << BOLD GREEN << "CGI script executed successfully." << RESET << std::endl;
        }
        catch (std::exception &e)
        {
            request._response->error(request, e.what(), client);
        }
        return;
    }

    bool isLocation = false;
    LocationFinder locationFinder;
    isLocation = locationFinder.locationMatch(client, request.getMethodPath(), socketFd);
    //-- SUMON I think this is not needed
    // if (!isLocation)
    // {
    //     request._response->error(socketFd, request, "404", client);
    //     return;
    // }

    //-- Check if the allowed methods include POST
    if (isLocation && locationFinder._allowedMethodFound)
    {
        if (locationFinder._allowed_methods.find("POST") == std::string::npos)
        {
            request._response->error(request, "405", client);
            return;
        }
    }

    //-- SUMON : Client Max Body Size check
    //-- Server directive
    std::string clientMaxBodySize;
    //-- If not found in server directive, then find in location directive
    clientMaxBodySize = findMaxBodySize(requestPath, client);
    if (clientMaxBodySize.empty())
    {
        clientMaxBodySize = client->_server->getServerConfig().getClientMaxBodySize();
        if (clientMaxBodySize.empty())
            clientMaxBodySize = "1";
    }

    //-- If the client_max_body_size is set to 0, then no limit
    if (clientMaxBodySize != "0")
    {
        size_t maxBodySize = std::atoi(clientMaxBodySize.c_str()) * 1024 * 1024;
        if (request._requestBody.size() > maxBodySize)
            throw std::runtime_error("413");
    }

    this->socketFd = socketFd;
    this->root = locationFinder._root;
    this->locationPath = locationFinder._locationPath;
    this->saveDir = root + locationPath + "/";
    this->fileBody = body;
    fileName = request._postFilename;

    handlePostRequest(request, client);
}

//-- SUMON : Client Max Body Size check
//-- this will return the max body size for the request path
std::string	PostMethod::findMaxBodySize(std::string requestPath, Client *client)
{
	LocationFinder locationFinder;

	bool locationMatched = locationFinder.locationMatch(client, requestPath, 0);
	if (!locationMatched)
		return ("1");

	if (locationFinder._clientBodySizeFound)
		return (locationFinder._clientMaxBodySize);
    return ("");
}

void PostMethod::handlePostRequest(Request &request, Client *client)
{
    std::string fileToCreate = saveDir + fileName;
    std::ofstream file;

    file.open(fileToCreate.c_str(), std::ios::binary);
    if (file.is_open())
    {
        file.write(fileBody.c_str(), fileBody.size());
        file.close();
    }
    else
    {
        std::cerr << BOLD RED << "Error: Could not open file for writing" << RESET << std::endl;
        request._response->error(request, "500", client);
        return;
    }
   
    std::string body =  " <html><head> <title>File uploaded</title>"
        "  <style> body { display: flex; justify-content: center;"
        "  align-items: center; height: 100vh; font-family: Arial, sans-serif;"
        "  font-size: 1em; font-weight: bold; font-style: italic; } </style>"
        "  </head> <body> <h1>File uploaded successfully!</h1><br>"
        "  <br><a href=\"" + locationPath + "/" + fileName + "\">VIEW</a>"
        "  </body> </html>";

    request._response->createHeaderAndBodyString(request, body, "200", client);

    //-- Remove the double slashes from the file path
    size_t pos = fileToCreate.find("//");
    if (pos != std::string::npos)
        fileToCreate.erase(pos, 1);
    
    //-- Print the file size
    size_t fileSize = fileBody.size();
    double fileSizeInMB = static_cast<double>(fileSize) / (1024.0 * 1024.0);

    std::cout << BOLD YELLOW << "size : " << fileSize << " BYTES  |  " << fileSize / 1024 << " KB  |  "  << std::fixed << std::setprecision(1) << fileSizeInMB << " MB" << RESET << std::endl;
    std::cout << BOLD BLUE "File : " << fileToCreate << RESET << std::endl;
    std::cout << BOLD GREEN "FILE SAVED! 💾" << RESET << std::endl;
}

Method*	PostMethod::clone()
{
	return new PostMethod(*this);
}

void PostMethod::setLocationPath(std::string locationPath)
{
    this->locationPath = locationPath;
}

void PostMethod::setRoot(std::string root)
{
    this->root = root;
}
