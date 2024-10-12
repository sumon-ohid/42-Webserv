#include "GetMethod.hpp"

GetMethod::GetMethod() : Method() { socketFd = -1; }

GetMethod::GetMethod(const GetMethod& other) : Method(other) {}

GetMethod&	GetMethod::operator=(const GetMethod& other) {
	if (this == &other)
		return *this;
	Method::operator=(other);
	return *this;
}

GetMethod::~GetMethod() {}

//-- This function can execute the request.
//-- Store the request path,
//-- compare it with location path.
//-- if match get root and index and other values.
void GetMethod::executeMethod(int _socketFd, Client *client, Request &request)
{
    socketFd = _socketFd;
    std::vector<LocationConfig> locationConfig = client->_server->_serverConfig.getLocations();
    std::string requestPath = request.getMethodPath();
    std::string locationPath;
    std::string root;
    std::string index;
    bool locationMatched = false;
    bool cgiFound = false;

    locationMatched = findMatchingLocation(locationConfig, requestPath, locationPath, root, index, cgiFound);
    if (locationMatched)
    {
        if (cgiFound)
            executeCgiScript(requestPath, client, request);
        else
            serveStaticFile(locationPath, request);
    }
    else
    {
        for (size_t i = 0; i < locationConfig.size(); i++)
        {
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it = locationMap.find("root");
            if (it != locationMap.end())
                root = it->second;
            std::string path = root + request.getMethodPath();

            serveStaticFile(path, request);
        }
    }
}

bool GetMethod::findMatchingLocation(std::vector<LocationConfig> &locationConfig, std::string &requestPath,
     std::string &locationPath, std::string &root, std::string &index, bool &cgiFound)
{
    for (size_t i = 0; i < locationConfig.size(); i++)
    {
        std::string tempPath = locationConfig[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        if (requestPath == tempPath || (requestPath.find(tempPath) == 0 && requestPath[tempPath.length()] == '/'))
        {
            if (requestPath.find("cgi-bin") != std::string::npos)
            {
                cgiFound = true;
                return true;
            }

            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it;

            for (it = locationMap.begin(); it != locationMap.end(); it++)
            {
                if (it->first == "root")
                    root = it->second;
                if (it->first == "index")
                    index = it->second;
                if (it->first == "return")
                {
                    handleRedirection(it->second);
                    return false;
                }
            }
            locationPath = (requestPath == "/") ? root + index : root + requestPath;
            return true;
        }
    }
    return false;
}

void GetMethod::handleRedirection(std::string &redirectUrl)
{
    std::cout << BOLD YELLOW << "Redirecting to: " << redirectUrl << RESET << std::endl;
    std::ostringstream redirectHeader;
    redirectHeader << "HTTP/1.1 301 Moved Permanently\r\n"
                   << "Location: " << redirectUrl << "\r\n"
                   << "Content-Length: 0\r\n"
                   << "Connection: close\r\n\r\n";
    std::string response = redirectHeader.str();
    ssize_t bytes_written = write(socketFd, response.c_str(), response.size());
    if (bytes_written == -1)
        throw std::runtime_error("Error writing to socket in GetMethod::handleRedirection!!");
    else
        std::cout << BOLD GREEN << "Redirect response sent successfully" << RESET << std::endl;
}

void GetMethod::serveStaticFile(std::string &path, Request &request)
{
    this->setMimeType(path);
    std::ifstream file(path.c_str());
    if (!file.is_open())
    {
        std::cerr << BOLD RED << "Error: 404 not found" << RESET << std::endl;
        Response::FallbackError(socketFd, request, "404");
        return;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    std::string body = buffer.str();
    file.close();
    Response::headerAndBody(socketFd, request, body);
    std::cout << BOLD GREEN << "Response sent to client successfully 🚀" << RESET << std::endl;
}

//-- Handle CGI script execution.
void GetMethod::executeCgiScript(std::string &requestPath, Client *client, Request &request)
{
    try
    {
        HandleCgi cgi(requestPath, socketFd, *client, request);
        std::cout << BOLD GREEN << "CGI script executed successfully." << RESET << std::endl;
    }
    catch (std::runtime_error &e)
    {
        std::cerr << BOLD RED << "Error: " << e.what() << RESET << std::endl;
        Response::FallbackError(socketFd, request, "404");
    }
}

Method*	GetMethod::clone() {
	return new GetMethod(*this);
}
