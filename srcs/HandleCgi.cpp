//-- Written by : msumon

#include "../includes/HandleCgi.hpp"
#include "../includes/Client.hpp"
#include "../includes/Request.hpp"
#include "../includes/Server.hpp"
#include "../includes/Helper.hpp"
#include "../includes/Response.hpp"
#include "../includes/LocationFinder.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <string>
#include <algorithm>
#include <sys/epoll.h>

HandleCgi::HandleCgi()
{
    _locationPath = "";
    _method = "";
    _postBody = "";
    _fileName = "";
	_pid = -1;
	_childReaped = false;
	_byteTracker = 0;
	_totalBytesSent = 0;
	_totalBytesSent = 0;
	_mimeCheckDone = false;
	_cgiDone = false;
}

//-- Function to initialize the environment variables
//-- We can add more environment variables here
void HandleCgi::initEnv(Request &request)
{
    _env["SERVER_PROTOCOL"] = request.getMethodProtocol();
    _env["GATEWAY_INTERFACE"] = "CGI/1.1";
    _env["SERVER_SOFTWARE"] = "Webserv/1.0";
    _env["METHOD"] = request.getMethodName();
    _env["PATH_INFO"] = _locationPath;
    _env["METHOD PATH"] = request.getMethodPath();
    _env["CONTENT_LENGTH"] = xto_string(request._requestBody.size());
    _env["CONTENT_TYPE"] = request.getMethodMimeType();
}

//-- Constructor to handle the CGI request
//-- I will parse the request to get the path of the CGI file
//-- then I will call the proccessCGI function to execute the CGI script
//-- and send the output to the client
HandleCgi::HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request)
{
    //-- Initialize the environment variables
    LocationFinder locationFinder;
    locationFinder.locationMatch(&client, requestBuffer, nSocket);
    std::string rootFolder = locationFinder._root;
    std::string location = locationFinder._locationPath;
    std::string index = locationFinder._index;
    _locationPath = locationFinder._pathToServe;

    //-- Handle POST method for CGI
    _method = request.getMethodName();
    _postBody = request._requestBody;
    _fileName = request._postFilename;
	_pid = -1;
	_childReaped = false;
	_byteTracker = 0;
	_totalBytesSent = 0;
	_mimeCheckDone = false;
	_cgiDone = false;
    if (locationFinder.isDirectory(_locationPath))
        throw std::runtime_error("404");
    if (_method != "GET" && _method != "POST")
        throw std::runtime_error("405");
    if (locationFinder._allowedMethodFound && locationFinder._cgiFound)
    {
        if (locationFinder._allowed_methods.find(_method) == std::string::npos)
            throw std::runtime_error("405");
    }
    client._isCgi = true;
    proccessCGI(&client);
}

//--- Main function to process CGI
void HandleCgi::proccessCGI(Client* client)
{
	if (pipe(_pipeOut) == -1 || pipe(_pipeIn) == -1)
		throw std::runtime_error("500");
	_pid = fork();
	if (_pid < 0)
		throw std::runtime_error("500");
	else if (_pid == 0)
		handleChildProcess(_locationPath, client->_request);
	else
		handleParentProcess(client);
}


//-- Function to handle the child process
void HandleCgi::handleChildProcess(const std::string &_locationPath, Request &request)
{
    dup2(_pipeIn[0], STDIN_FILENO); //-- Redirect stdin to read end of the pipe
    dup2(_pipeOut[1], STDOUT_FILENO); //-- Redirect stdout to write end of the pipe

    close(_pipeIn[0]); //-- Close write end of the pipe
    close(_pipeOut[1]); //-- Close read end of the pipe

    initEnv(request);
    std::string executable = getExecutable(_locationPath);

    //--- Prepare arguments for execve
    char *const argv[] = { (char *)executable.c_str(), (char *)_locationPath.c_str(), NULL };

    //--- Prepare environment variables for execve
    std::vector<char*> envp;
    for (std::map<std::string, std::string>::const_iterator it = _env.begin(); it != _env.end(); it++)
    {
        std::string envVar = it->first + "=" + it->second + '\n';
        char* envStr = new char[envVar.size() + 1];
        std::copy(envVar.begin(), envVar.end(), envStr);
        envStr[envVar.size()] = '\0';
        envp.push_back(envStr);
    }
    envp.push_back(NULL);

    execve(executable.c_str(), argv, envp.data());
    std::cout << BOLD RED << "EXECVE FAILED" << RESET << std::endl;
    throw std::runtime_error("500");
}

//-- Function to determine the executable based on the file extension
std::string HandleCgi::getExecutable(const std::string &_locationPath)
{
    size_t pos = _locationPath.rfind(".");
    std::string extension;
    if (pos != std::string::npos)
        extension = _locationPath.substr(pos);
    else
        throw std::runtime_error("403"); //-- NOT SURE IF 403 IS THE RIGHT ERROR CODE

    std::string executable;
    executable = Helper::executableMap.find(extension)->second;
    if (executable.empty())
        throw std::runtime_error("403"); //-- NOT SURE IF 403 IS THE RIGHT ERROR CODE

    return executable;
}

//----- Function to handle the parent process
//--- NOTE: This should go through epoll
//--- read there from pipe_fd[0] and write to the client socket
void HandleCgi::handleParentProcess(Client* client)
{
	close(_pipeIn[0]); //-- Close read end of the pipe
	close(_pipeOut[1]); //-- Close write end of the pipe
	client->_epoll->registerSocket(_pipeIn[1], EPOLLOUT);
	client->_epoll->addCgiClientToEpollMap(_pipeIn[1], client);
}

void	HandleCgi::checkReadOrWriteError(Client* client, int fdToClose)
{
	if (_byteTracker > -1)
		return;
	std::cout << strerror(errno) << std::endl;
	client->_epoll->removeCgiClientFromEpoll(fdToClose);
	throw std::runtime_error("500");
}

void	HandleCgi::writeToChildFd(Client* client)
{
	_response = std::vector<char>(client->_request._requestBody.begin(), client->_request._requestBody.end());
	_byteTracker = write(_pipeIn[1], _response.data() + _totalBytesSent, _response.size() - _totalBytesSent);
	_totalBytesSent += _byteTracker;
	checkReadOrWriteError(client, _pipeIn[1]);
	if (_byteTracker == 0)
		finishWriteAndPrepareReadFromChild(client);
	_byteTracker = 0;
}

void	HandleCgi::finishWriteAndPrepareReadFromChild(Client* client)
{
	client->_epoll->removeCgiClientFromEpoll(_pipeIn[1]);
	client->_epoll->registerSocket(_pipeOut[0], EPOLLIN);
    client->_epoll->addCgiClientToEpollMap(_pipeOut[0], client);
	_byteTracker = 0;
	_totalBytesSent = 0;
	_response.clear();
	client->_request._response->setIsChunk(true);
}

void	HandleCgi::processCgiDataFromChild(Client* client)
{
	checkWaitPid();
	readFromChildFd();
	checkReadOrWriteError(client, _pipeOut[0]);
	if (_byteTracker == 0)
		finishReadingFromChild(client);
	if (!_mimeCheckDone)
		MimeTypeCheck(client);
	else
		_responseStr = std::string(_response.data(), _byteTracker);
	client->_request._response->createHeaderAndBodyString(client->_request, _responseStr, "200", client);
	_byteTracker = 0;
}

void	HandleCgi::checkWaitPid()
{
	if (_childReaped)
		return;
	int status = 0;
	pid_t result = waitpid(_pid, &status, WNOHANG);
	if (result == -1)
		throw (500); 
	else if (result > 0) 
	{
		// Child process has terminated
		if (WIFEXITED(status))
			std::cout << "Child exited with status: " << WEXITSTATUS(status) << std::endl;
		else if (WIFSIGNALED(status))
			std::cerr << "Child terminated by signal: " << WTERMSIG(status) << std::endl;
		_childReaped = true;
	}
}

void	HandleCgi::readFromChildFd()
{
	_response.resize(64000, '\0');
	_byteTracker = read(_pipeOut[0], _response.data(), _response.size());
	_totalBytesSent += _byteTracker;
}

void	HandleCgi::finishReadingFromChild(Client* client)
{
	client->_epoll->removeCgiClientFromEpoll(_pipeOut[0]);
	if (!_mimeCheckDone)
		MimeTypeCheck(client);
	_cgiDone = true;
}

void	HandleCgi::MimeTypeCheck(Client* client)
{
	//*** This is to handle mime types for cgi scripts
	_responseStr = std::string(_response.data(), _byteTracker);
	size_t pos = _responseStr.find("Content-Type:");
	std::string setMime;
	extractMimeType(pos, setMime);
	client->_request.setMethodMimeType(setMime);
	_mimeCheckDone = true;
	size_t bodyStart = _responseStr.find("\r\n\r\n");
	if (bodyStart != std::string::npos)
		_responseStr.erase(0, bodyStart += 5);
}

void	HandleCgi::extractMimeType(size_t pos, std::string& setMime)
{
	if (pos != std::string::npos)
	{
		std::string mimeType = _responseStr.substr(pos + 14, _responseStr.find("\r\n", pos) - pos - 14);

		std::map<std::string, std::string> mimeTypes = Helper::mimeTypes;
		std::map<std::string, std::string>::iterator it;
		for (it = mimeTypes.begin(); it != mimeTypes.end(); it++)
		{
			if (mimeType == it->second)
			{
				setMime = it->first;
				break;
			}
		}
	}
	if (setMime.empty())
    	setMime = ".html";
}

void	HandleCgi::closeCgi(Client* client)
{
	client->_epoll->removeCgiClientFromEpoll(_pipeIn[1]);
	client->_epoll->removeCgiClientFromEpoll(_pipeOut[0]);
}

bool    HandleCgi::getCgiDone() const
{
    return (_cgiDone);
}

HandleCgi::~HandleCgi()
{
    _env.clear();
}


//--- Copy constructor
HandleCgi::HandleCgi(const HandleCgi &src)
	:	_locationPath(src._locationPath),
		_method(src._method),
		_postBody(src._postBody),
		_fileName(src._fileName),
		_pid(src._pid),
		_childReaped(src._childReaped),
		_byteTracker(src._byteTracker),
		_totalBytesSent(src._totalBytesSent),
		_response(src._response),
		_responseStr(src._responseStr),
		_mimeCheckDone(src._mimeCheckDone),
		_cgiDone(src._cgiDone),
		_env(src._env)
{
	// Deep copy the pipe file descriptors
	_pipeIn[0] = src._pipeIn[0];
	_pipeIn[1] = src._pipeIn[1];
	_pipeOut[0] = src._pipeOut[0];
	_pipeOut[1] = src._pipeOut[1];
}

//--- Assignment operator
HandleCgi &HandleCgi::operator=(const HandleCgi &src)
{
    if (this != &src)
	{
		_locationPath = src._locationPath;
		_method = src._method;
		_postBody = src._postBody;
		_fileName = src._fileName;
		_pipeIn[0] = src._pipeIn[0];
		_pipeIn[1] = src._pipeIn[1];
		_pipeOut[0] = src._pipeOut[0];
		_pipeOut[1] = src._pipeOut[1];
		_pid = src._pid;
		_childReaped = src._childReaped;
		_byteTracker = src._byteTracker;
		_totalBytesSent = src._totalBytesSent;
		_response = src._response;
		_responseStr = src._responseStr;
		_mimeCheckDone = src._mimeCheckDone;
		_cgiDone = src._cgiDone;
		_env = src._env;
	}
	return (*this);
}

//--- == operator overloading
bool HandleCgi::operator==(const HandleCgi &src) const
{
    return (_locationPath == src._locationPath &&
           _method == src._method &&
           _postBody == src._postBody &&
           _fileName == src._fileName &&
           _pipeIn[0] == src._pipeIn[0] &&
           _pipeIn[1] == src._pipeIn[1] &&
           _pipeOut[0] == src._pipeOut[0] &&
           _pipeOut[1] == src._pipeOut[1] &&
		   _pid == src._pid &&
		   _childReaped == src._childReaped &&
           _byteTracker == src._byteTracker &&
           _totalBytesSent == src._totalBytesSent &&
           _response == src._response &&
           _responseStr == src._responseStr &&
           _mimeCheckDone == src._mimeCheckDone &&
           _cgiDone == src._cgiDone &&
           _env == src._env);
}
