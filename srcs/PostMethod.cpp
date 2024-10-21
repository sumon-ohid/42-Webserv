
#include "../includes/PostMethod.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <fstream>

PostMethod::PostMethod() : socketFd(-1)
{
    saveDir = "";
    fileName = "";
    fileBody = "";
    root = "";
    locationPath = "";
}

PostMethod::PostMethod(const PostMethod& other) : socketFd(other.socketFd) {}

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
    std::string body = "------WebKitFormBoundaryhLw0c39BS7tY0rrh\r\n"
                       "Content-Disposition: form-data; name=\"file\"; filename=\"testUpload.txt\"\r\n"
                       "Content-Type: text/plain\r\n"
                       "\r\n"
                       "asdf\r\n"
                       "asdf\r\n"
                       "jfjfjfjfj\r\n"
                       "halloe!\r\n";
    
    std::string requestPath = request.getMethodPath();
    this->socketFd = socketFd;
    this->saveDir = root + locationPath + "/";
    this->fileBody = parseBody(body);
    fileName = parseFilename(body);

    if (requestPath == "/submit" || requestPath == "/upload")
    {
        handlePostRequest(request, client);
    }
    else
        Response::error(socketFd, request, "404", client);
}

void PostMethod::handlePostRequest(Request &request, Client *client)
{
    (void) request;
    (void) client;
    std::string fileToCreate = saveDir + fileName;
    std::ofstream file;

    file.open(fileToCreate.c_str());
    file << fileBody;
    file.close();

    std::string body = "<html><body><h1>File uploaded successfully!</h1></body></html>";
    Response::headerAndBody(socketFd, request, body);
    
    std::cout << BOLD BLUE "File: " << fileToCreate << RESET << std::endl;
    std::cout << BOLD GREEN "FILE SAVED! 💾" << RESET << std::endl;
}

std::string PostMethod::parseBody(std::string body)
{
    size_t pos = body.find("\r\n\r\n");
    if (pos == std::string::npos)
        throw std::runtime_error("Error: Invalid body");

    return body.substr(pos + 4);
}

std::string PostMethod::parseFilename(std::string& content)
{
    std::string filename;
    std::string::size_type pos = content.find("filename=\"");
    if (pos != std::string::npos)
    {
        pos += 10; // Move past 'filename="'
        std::string::size_type endPos = content.find("\"", pos);
        if (endPos != std::string::npos)
        {
            filename = content.substr(pos, endPos - pos);
        }
    }
    return filename;
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
