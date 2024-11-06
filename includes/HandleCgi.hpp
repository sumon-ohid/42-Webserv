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

class Client;

class HandleCgi : public ServerConfig
{
    private:
        std::string							_locationPath;
        std::string							_method;
        std::string							_postBody;
        std::string							_fileName;
        int									_pipeIn[2];
		int									_pipeOut[2];
		ssize_t								_byteTracker;
		ssize_t								_totalBytesSent;
		std::vector<char>					_response;
		std::string							_responseStr;
		bool								_mimeCheckDone;
        bool                                _cgiDone;
        std::map<std::string, std::string>	_env;

    public:
        HandleCgi();
        HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request);
        ~HandleCgi();

        void			initEnv(Request &request);
        void			proccessCGI(Client*);
        void			handleChildProcess(const std::string &_locationPath, Request &request);
        std::string		getExecutable(const std::string &locationPath);
        void			handleParentProcess(Client* client);
		void			writeToChildFd(Client* client);
		void			readFromChildFd(Client* client);
		void			MimeTypeCheck(Client *client);

        bool            getCgiDone() const;

        // Function template to convert various types to string
        template <typename T>
        std::string xto_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
