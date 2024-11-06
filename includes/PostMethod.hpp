#pragma once 

#include "Method.hpp"

#include <string>

class PostMethod : public Method
{
    private:
        int socketFd;
        std::string saveDir;
        std::string fileName;
        std::string fileBody;

        std::string root;
        std::string locationPath;

    public:
        PostMethod();
        PostMethod(const PostMethod& other);
        PostMethod& operator=(const PostMethod& other);
        ~PostMethod();

        void setRoot(std::string root);
        void setLocationPath(std::string locationPath);

        void executeMethod(int socketFd, Client *client, Request &request);
        void handlePostRequest(Request &request, Client *client);
        std::string parseBody(std::string body);
        std::string parseFilename(std::string& content);

        //-- SUMON : Client Max Body Size check
		std::string findMaxBodySize(std::string requestPath, Client* client);

        Method*	clone();
};
