//-- Written by : msumon

#pragma once

#include <map>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sstream>

#include "Request.hpp"
#include "ServerConfig.hpp"
#include "Server.hpp"

class HandleCgi : public ServerConfig
{
    private:
        std::string locationPath;
        std::string method;
        std::string postBody;
        std::string fileName;
        std::map<std::string, std::string> env;

    public:
        HandleCgi();
        HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request);
        ~HandleCgi();

        void initEnv(Request &request);
        void proccessCGI(Client*, int nSocket, Request &request);
        void handleParentProcess(Client*, int nSocket, int pipe_in[2], int pipe_out[2], pid_t pid, Request &request);
        void handleChildProcess(int pipe_in[2], int pipe_out[2] ,const std::string &locationPath, Request &request);
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
