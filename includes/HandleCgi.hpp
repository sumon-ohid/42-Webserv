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
	int									_pipeOut[2];
	pid_t								_pid;
	bool								_childReaped;
	ssize_t								_byteTracker;
	ssize_t								_totalBytesSent;
	std::vector<char>					_response;
	std::string							_responseStr;
	bool								_mimeCheckDone;
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
		void			handleChildProcess(const std::string&, Request&);
		std::string		getExecutable(const std::string&);
		void			handleParentProcess(Client*);
		void			checkReadOrWriteError(Client*, int);
		void			writeToChildFd(Client* );
		void			finishWriteAndPrepareReadFromChild(Client*);
		void			processCgiDataFromChild(Client*);
		void			checkWaitPid(Client*);
		void			readFromChildFd();
		bool			checkCgiTimeout(Client*);
		void			finishReadingFromChild(Client*);
		// void			MimeTypeCheck(Client *client);
		void			extractMimeType(size_t, std::string&);
		void			closeCgi(Client*);

		void			setCgiDone(bool);
		bool			getCgiDone() const;
		bool			getChildReaped() const;
		int				getPipeIn(unsigned) const;
		int				getPipeOut(unsigned) const;
		pid_t			getPid(void) const; 
		std::string		getLocationPath() const;

        // Function template to convert various types to string
        template <typename T>
        std::string xto_string(const T& value)
        {
            std::stringstream ss;
            ss << value;
            return ss.str();
        }
};
