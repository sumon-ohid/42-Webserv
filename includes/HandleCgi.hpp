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

class Client;

class HandleCgi
{
	private:
	std::string							_locationPath;
	std::string							_method;
	std::string							_postBody;
	std::string							_fileName;
	int									_pipeIn[2];
	int								_pipeOut[2];
	ssize_t							_byteTracker;
	ssize_t							_totalBytesSent;
	std::vector<char>				_response;
	std::string						_responseStr;
	bool							_mimeCheckDone;
	bool								_cgiDone;
	std::map<std::string, std::string>	_env;

	public:
		HandleCgi();
		HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request);
		~HandleCgi();

		//-- Copy constructor
		HandleCgi(const HandleCgi &src);
		//-- Assignment operator
		HandleCgi &operator=(const HandleCgi &src);
		//-- == operator overloading
		bool operator==(const HandleCgi &src) const;

		void			initEnv(Request &request);
		void			proccessCGI(Client*);
		void			handleChildProcess(const std::string &_locationPath, Request &request);
		std::string		getExecutable(const std::string &locationPath);
		void			handleParentProcess(Client* client);
		void			checkReadOrWriteError(Client*, int);
		void			writeToChildFd(Client* client);
		void			finishWriteAndPrepareReadFromChild(Client*);
		void			processCgiDataFromChild(Client*);
		void			readFromChildFd();
		void			finishReadingFromChild(Client* client);
		void			MimeTypeCheck(Client *client);
		void			extractMimeType(size_t pos, std::string& setMime);
		void			closeCgi(Client* client);

		void			setCgiDone(bool);
		bool			getCgiDone() const;
		int				getPipeIn(unsigned i) const;
		int				getPipeOut(unsigned i) const;

        // Function template to convert various types to string
        template <typename T>
        std::string xto_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
