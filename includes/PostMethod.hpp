#pragma once 

#include "Method.hpp"

class PostMethod : public Method
{
    private:
        int socketFd;

    public:
        PostMethod();
        PostMethod(const PostMethod& other);
        PostMethod& operator=(const PostMethod& other);
        ~PostMethod();

        void executeMethod(int socketFd, Client *client, Request &request);
        void handlePostRequest(Request &request, Client *client);
};