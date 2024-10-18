//-- Written by : msumon

#include "HandleCgi.hpp"
#include "Client.hpp"
#include "Request.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"
#include "Helper.hpp"

#include <cstddef>
#include <string>

HandleCgi::HandleCgi()
{
    locationPath = "";
}

//-- Constructor to handle the CGI request
//-- I will parse the request to get the path of the CGI file
//-- then I will call the proccessCGI function to execute the CGI script
//-- and send the output to the client
HandleCgi::HandleCgi(std::string requestBuffer, int nSocket, Client &client, Request &request)
{
    //--NOTE: cgiConf should have root in the beginning
    //-- parse location config to get the root here

    (void)request;
    std::string rootFolder;
    std::string root;
    std::string index;

    std::vector<LocationConfig> locationConfig = client._server->_serverConfig.getLocations();
    for (size_t i = 0; i < locationConfig.size(); i++)
    {
        std::string tempPath = locationConfig[i].getPath();
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), ' '), tempPath.end());
        tempPath.erase(std::remove(tempPath.begin(), tempPath.end(), '{'), tempPath.end());

        if (tempPath == "/")
        {
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            std::multimap<std::string, std::string>::iterator it = locationMap.find("root");
            if (it != locationMap.end())
                rootFolder = it->second;
            i++;
        }
        if (tempPath == "/cgi-bin")
        {
            std::multimap<std::string, std::string> locationMap = locationConfig[i].getLocationMap();
            for (std::multimap<std::string, std::string>::iterator it = locationMap.begin(); it != locationMap.end(); ++it)
            {
                if (it->first == "index")
                    index = it->second;
                if (it->first == "root")
                    root = it->second;
            }
            locationPath = rootFolder + root + index;
        }
    }

    if (requestBuffer == "/cgi-bin" || requestBuffer == "/cgi-bin/" || requestBuffer == root + index)
        proccessCGI(nSocket);
    else
        throw std::runtime_error("404");
}

//--- Main function to process CGI
void HandleCgi::proccessCGI(int nSocket)
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
        handleParentProcess(nSocket, pipe_fd, pid);
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
    char *const envp[] = { NULL };

    execve(executable.c_str(), argv, envp);
    throw std::runtime_error("Execve failed !!");
}

//----- Function to handle the parent process
void HandleCgi::handleParentProcess(int nSocket, int pipe_fd[2], pid_t pid)
{
    close(pipe_fd[1]); //--- Close unused write end

    //--- Read CGI output from the pipe
    std::vector<char> cgiOutput(1024);
    waitpid(pid, NULL, 0); //--- Wait for the child process to finish
    ssize_t n = read(pipe_fd[0], cgiOutput.data(), cgiOutput.size());
    if (n < 0)
        throw std::runtime_error("Read failed !!"); // BP: close fd[0]
    cgiOutput.resize(n);
    std::ostringstream httpRequest;
    httpRequest << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << n << "\n\n";

    //--- Send the HTTP request
    send(nSocket, httpRequest.str().c_str(), httpRequest.str().size(), 0);
    //--- Send the CGI output
    send(nSocket, cgiOutput.data(), cgiOutput.size(), 0);

    close(pipe_fd[0]); //--- Close read end
}

HandleCgi::~HandleCgi()
{
}
