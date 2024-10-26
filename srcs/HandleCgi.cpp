//-- Written by : msumon

#include "../includes/HandleCgi.hpp"
#include "../includes/Client.hpp"
#include "../includes/Request.hpp"
#include "../includes/Server.hpp"
#include "../includes/Helper.hpp"
#include "../includes/PostMethod.hpp"
#include "../includes/Response.hpp"
#include "../includes/LocationFinder.hpp"

#include <cstddef>
#include <string>
#include <algorithm>

HandleCgi::HandleCgi()
{
    locationPath = "";
    method = "";
    postBody = "";
    fileName = "";
    env.clear();
}

//-- Function to initialize the environment variables
//-- We can add more environment variables here
void HandleCgi::initEnv(Request &request)
{
    env["SERVER_PROTOCOL"] = request.getMethodProtocol();
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";
    env["METHOD"] = request.getMethodName();
    env["SCRIPT_NAME"] = request._postFilename;
    env["METHOD PATH"] = request.getMethodPath();
    env["CONTENT_LENGTH"] = xto_string(request._requestBody.size());
    env["CONTENT_TYPE"] = request.getMethodMimeType();
}

//-- Constructor to handle the CGI request
//-- I will parse the request to get the path of the CGI file
//-- then I will call the proccessCGI function to execute the CGI script
//-- and send the output to the client
HandleCgi::HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request)
{
    //-- Initialize the environment variables
    initEnv(request);

    LocationFinder locationFinder;
    locationFinder.locationMatch(&client, requestBuffer, nSocket);
    std::string rootFolder = locationFinder._root;
    std::string location = locationFinder._locationPath;
    std::string index = locationFinder._index;
    locationPath = rootFolder + location + "/" + index;

    //-- Handle POST method for CGI
    method = request.getMethodName();
    postBody = request._requestBody;
    fileName = request._postFilename;

    if (locationFinder.isDirectory(locationPath))
        throw std::runtime_error("404");
    if (method != "GET" && method != "POST")
        throw std::runtime_error("405");

    if (locationFinder._allowedMethodFound && locationFinder._cgiFound)
    {
        if (locationFinder._allowed_methods.find(method) == std::string::npos)
            throw std::runtime_error("405");
    }

    if (method == "POST")
    {
        PostMethod postMethod;
        postMethod.executeMethod(nSocket, &client, request);
        return;
    }
    if (requestBuffer == "/cgi-bin" || requestBuffer == "/cgi-bin/" || requestBuffer == location + index)
        proccessCGI(nSocket, request);
    else
        throw std::runtime_error("404");
}

//--- Main function to process CGI
void HandleCgi::proccessCGI(int nSocket, Request &request)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
        throw std::runtime_error("Pipe failed !!"); // BP: maybe 500 or where catch it throw 500

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("Fork failed !!"); // BP: maybe 500 or where catch it throw 500
    else if (pid == 0)
        handleChildProcess(pipe_fd, locationPath);
    else
        handleParentProcess(nSocket, pipe_fd, pid, request);
}

//-- Function to determine the executable based on the file extension
std::string HandleCgi::getExecutable(const std::string &locationPath)
{
    size_t pos = locationPath.rfind(".");
    std::string extension;
    if (pos != std::string::npos)
        extension = locationPath.substr(pos);
    else
        throw std::runtime_error("Invalid file extension !!"); 

    std::string executable;
    executable = Helper::executableMap.find(extension)->second;
    if (executable.empty())
        throw std::runtime_error("This file extension is not supported !!");

    return executable;
}

//-- Function to handle the child process
void HandleCgi::handleChildProcess(int pipe_fd[2], const std::string &locationPath)
{
    //--- Redirect stdout to the pipe's write end
    dup2(pipe_fd[1], STDOUT_FILENO);
    close(pipe_fd[0]); //--- Close unused read end
    close(pipe_fd[1]); //--- Close write end after redirection

    std::string executable = getExecutable(locationPath);

    //--- Prepare arguments for execve
    char *const argv[] = { (char *)executable.c_str(), (char *)locationPath.c_str(), NULL };
    
    //--- Prepare environment variables for execve
    std::vector<char*> envp;
    for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); it++)
    {
        std::string envVar = it->first + "=" + it->second + '\n';
        char* envStr = new char[envVar.size() + 1];
        std::copy(envVar.begin(), envVar.end(), envStr);
        envStr[envVar.size()] = '\0';
        envp.push_back(envStr);
    }
    envp.push_back(NULL);

    execve(executable.c_str(), argv, envp.data());
    throw std::runtime_error("Execve failed !!");
}

//----- Function to handle the parent process
void HandleCgi::handleParentProcess(int nSocket, int pipe_fd[2], pid_t pid, Request &request)
{
    (void)request;
    close(pipe_fd[1]); //--- Close unused write end

    //--- Read CGI output from the pipe
    std::vector<char> cgiOutput(SOCKET_BUFFER_SIZE);
    waitpid(pid, NULL, 0); //--- Wait for the child process to finish
    ssize_t n = read(pipe_fd[0], cgiOutput.data(), cgiOutput.size());
    if (n < 0)
    {
        close(pipe_fd[0]);
        throw std::runtime_error("Read failed at CGI !!");
    }
    else if (n == 0)
    {
        close(pipe_fd[0]);
        throw std::runtime_error("No data received from CGI script !!");
    }

    cgiOutput.resize(n);

    std::string body = xto_string(cgiOutput.data());

    //*** This is to handle mime types for cgi scripts
    // size_t pos = body.find("Content-Type:");
    // std::string mimeType = body.substr(pos + 14, body.find("\n", pos) - pos - 14);
    // std::ostringstream httpRequest;
    // httpRequest << "HTTP/1.1 200 OK\nContent-Type: "<< mimeType <<"\nContent-Length: " << n << "\n\n";

    // //--- Send the HTTP request
    // send(nSocket, httpRequest.str().c_str(), httpRequest.str().size(), 0);
    // //--- Send the CGI output
    // send(nSocket, cgiOutput.data(), cgiOutput.size(), 0);

    Response::headerAndBody(nSocket, request, body);

    close(pipe_fd[0]); //--- Close read end
}

HandleCgi::~HandleCgi()
{
    env.clear();
}
