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
	_byteTracker = 0;
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
	_byteTracker = 0;
	_mimeCheckDone = false;

    if (locationFinder.isDirectory(_locationPath))
        throw std::runtime_error("404");
    if (_method != "GET" && _method != "POST")
        throw std::runtime_error("405");

    if (locationFinder._allowedMethodFound && locationFinder._cgiFound)
    {
        if (locationFinder._allowed_methods.find(_method) == std::string::npos)
            throw std::runtime_error("405");
    }

    // if (method == "POST")
    // {
    //     // PostMethod postMethod;
    //     // postMethod.executeMethod(nSocket, &client, request);
    //     // return;
    //     proccessCGI(&client, nSocket, request);
    // }

    // if (requestBuffer == "/cgi-bin" || requestBuffer == "/cgi-bin/" || requestBuffer == location + index)
    //     proccessCGI(&client, nSocket, request);
    // else
    //     throw std::runtime_error("404");
    client._isCgi = true;
    proccessCGI(&client, nSocket, request);
}

//--- Main function to process CGI
void HandleCgi::proccessCGI(Client* client, int nSocket, Request &request)
{
    if (pipe(_pipeOut) == -1 || pipe(_pipeIn) == -1)
        throw std::runtime_error("500");
    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("500");
    else if (pid == 0)
        handleChildProcess(_locationPath, request);
    else
        handleParentProcess(client, nSocket, pid, request);
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
void HandleCgi::handleParentProcess(Client* client, int nSocket, pid_t pid, Request &request)
{
    close(_pipeIn[0]); //-- Close read end of the pipe
    close(_pipeOut[1]); //-- Close write end of the pipe
    // std::cout << BOLD YELLOW << "REQUEST BODY: " << request._requestBody << RESET << std::endl;
	_response = std::vector<char>(client->_request._requestBody.begin(), client->_request._requestBody.end());
	std::cout << _response << std::endl;
    client->_epoll->registerSocket(_pipeIn[1], EPOLLOUT);
    client->_epoll->addCgiClientToEpollMap(_pipeIn[1], client);
	(void)nSocket;
	(void)pid;
	(void)request;
}



    // -- Write the body to the CGI script's stdin
    // Write the body to the CGI script's stdin


    // ssize_t bytes_written = write(_pipeIn[1], response.data(), response.size());
    // if (bytes_written == -1)
    // {
    //     close(_pipeIn[1]);
    //     throw std::runtime_error("500");
    // }
    // // std::cout << "Bytes written to pipe: " << bytes_written << std::endl;
    // // std::cout << "Data written to pipe: " << std::string(response.begin(), response.end()) << std::endl;


    // close(_pipeIn[1]); //-- Close write end after sending data

    // //--- Read CGI output from the pipe

    // client->_epoll->registerSocket(_pipeOut[0], EPOLLIN | EPOLLET);
    // client->_epoll->addCgiClientToEpoll(_pipeOut[0], client);

    // std::ostringstream bodyStr;
    // int status;
    // std::vector<char> cgiOutput(64000); //-- Buffer size of 64 KB
    // while (true)
    // {
    //     ssize_t n = read(_pipeOut[0], cgiOutput.data(), cgiOutput.size());
    //     if (n > 0)
    //         bodyStr.write(cgiOutput.data(), n);
    //     else if (n == 0)
    //         break;
    //     else
    //     {
    //         close(_pipeOut[0]);
    //         throw std::runtime_error("500");
    //     }

    //     //-- Check if the child process has exited
    //     //-- WNOHANG: return immediately if no child has exited
    //     if (waitpid(pid, &status, WNOHANG) == pid)
    //     {
    //         if (WIFEXITED(status))
    //             std::cout << BOLD BLUE << "Process completed with exit code: " << WEXITSTATUS(status) << RESET << std::endl;
    //         break;
    //     }
    // }

void	HandleCgi::writeToChildFd(Client* client)
{
    // std::cout << BOLD YELLOW << "REQUEST BODY: " << request._requestBody << RESET << std::endl;
    //-- Write the body to the CGI script's stdin
    // Write the body to the CGI script's stdin
	std::cout << "cgi writing to child" << std::endl;
	_byteTracker = write(_pipeIn[1], _response.data() + _byteTracker, _response.size() - _byteTracker);
	_totalBytesSent += _byteTracker;
	if	(_byteTracker == -1)
    {
        close(_pipeIn[1]);
        throw std::runtime_error("500");
    }
	else if (_byteTracker == 0)
	{
		std::cout << "0 bytes written in cgi writeToChildFd" << std::endl;
		client->_epoll->removeCgiClientFromEpoll(_pipeIn[1]);
		client->_epoll->registerSocket(_pipeOut[0], EPOLLIN | EPOLLET); //register child's output pipe for writtening
    	client->_epoll->addCgiClientToEpollMap(_pipeOut[0], client); //
		_byteTracker = 0;
		_totalBytesSent = 0;
		_response.clear();
		client->_request._response->setIsChunk(true);
	}
	else if(static_cast<unsigned long>(_totalBytesSent) == _response.size())
	{
		std::cout << "All bytes written in cgi writeToChildFd" << std::endl;
		client->_epoll->removeCgiClientFromEpoll(_pipeIn[1]);
		client->_epoll->registerSocket(_pipeOut[0], EPOLLIN | EPOLLET); //register child's output pipe for reading
    	client->_epoll->addCgiClientToEpollMap(_pipeOut[0], client); //
		_byteTracker = 0;
		_totalBytesSent = 0;
		_response.clear();
		client->_request._response->setIsChunk(true);
	}
	// std::cout << "Bytes written to pipe: " << bytes_written << std::endl;
    // std::cout << "Data written to pipe: " << std::string(response.begin(), response.end()) << std::endl;

}

void	HandleCgi::readFromChildFd(Client* client)
{
	// std::ostringstream bodyStr;
	// std::ostringstream bodyStr;


    // std::vector<char> cgiOutput(64000); //-- Buffer size of 64 KB
	std::cout << "cgi reading from child" << std::endl;
	_response.resize(64000, '\0');
	_byteTracker = read(_pipeOut[0], _response.data(), _response.size());
	_totalBytesSent += _byteTracker;
	std::cout << "bytes read from child:\t" << _byteTracker << std::endl;
	if	(_byteTracker == -1)
    {
        std::cout << strerror(errno) << std::endl;
		close(_pipeOut[0]);
        throw std::runtime_error("500");
    }
	else if (_byteTracker == 0)
	{
		std::cout << "0 bytes read in cgi readFromChild" << std::endl;
		client->_epoll->removeCgiClientFromEpoll(_pipeOut[0]);
		if (!_mimeCheckDone)
			MimeTypeCheck(client);
		_responseStr = std::string(_response.begin(), _response.end());
		client->_request._response->createHeaderAndBodyString(client->_request, _responseStr, "200", client);
        _cgiDone = true;
		_byteTracker = 0;
	}
	if (!_mimeCheckDone)
		MimeTypeCheck(client);
	// Helper::modifyEpollEvent(*client->_epoll, client, EPOLLIN);
	_responseStr = std::string(_response.data(), _byteTracker);
	std::cout << "length of response string:\t" << _responseStr.length() << std::endl;
	client->_request._response->createHeaderAndBodyString(client->_request, _responseStr, "200", client);
	// client->_request._response->addToBody(std::string(_response.begin(), _response.end()));
	// Helper::modifyEpollEvent(*client->_epoll, client, EPOLLOUT);
}

void	HandleCgi::MimeTypeCheck(Client* client)
{
	std::string body(_response.begin(), _response.end());
    //*** This is to handle mime types for cgi scripts
    size_t pos = body.find("Content-Type:");
    std::string mimeType = body.substr(pos + 14, body.find("\r\n", pos) - pos - 14);
    std::string setMime;

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
    //-- If no mime types found set it to default
    if (setMime.empty())
        setMime = ".html";
    client->_request.setMethodMimeType(setMime);
	_mimeCheckDone = true;
	size_t bodyStart = body.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
    {
        bodyStart += 5;
        body.erase(0, bodyStart);
    }
}

bool    HandleCgi::getCgiDone() const
{
    return (_cgiDone);
}

HandleCgi::~HandleCgi()
{
    _env.clear();
}
