
#include "../includes/PostMethod.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"

PostMethod::PostMethod() : socketFd(-1) {}

PostMethod::PostMethod(const PostMethod& other) : socketFd(other.socketFd) {}

PostMethod& PostMethod::operator=(const PostMethod& other)
{
    if (this == &other)
        return (*this);
    socketFd = other.socketFd;
    return (*this);
}

PostMethod::~PostMethod() {}

//-- Execute the POST method
void PostMethod::executeMethod(int socketFd, Client *client, Request &request)
{
    this->socketFd = socketFd;
    std::string requestPath = request.getMethodPath();

    if (requestPath == "/submit" || requestPath == "/upload")
    {
        handlePostRequest(request, client);
    }
    else
        Response::error(socketFd, request, "404", client);
}

void PostMethod::handlePostRequest(Request &request, Client *client)
{
    
}