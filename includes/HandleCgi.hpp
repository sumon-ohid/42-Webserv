//-- Written by : msumon

#pragma once

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sstream>

#include "ServerConfig.hpp"
#include "Server.hpp"

class HandleCgi : public ServerConfig
{
    private:
        std::string locationPath;
        std::string method;
        std::string postBody;
        std::string fileName;

    public:
        HandleCgi();
        HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request);
        ~HandleCgi();

        void proccessCGI(int nSocket);
        void handleParentProcess(int nSocket, int pipe_fd[2], pid_t pid);
        void handleChildProcess(int pipe_fd[2], const std::string &locationPath);
        std::string getExecutable(const std::string &locationPath);
        
        // Function template to convert various types to string
        template <typename T>
        std::string xto_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
