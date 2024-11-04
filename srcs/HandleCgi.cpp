//-- Written by : msumon

#include "../includes/HandleCgi.hpp"
#include "../includes/Client.hpp"
#include "../includes/Request.hpp"
#include "../includes/Server.hpp"
#include "../includes/Helper.hpp"
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
}

//-- Function to initialize the environment variables
//-- We can add more environment variables here
void HandleCgi::initEnv(Request &request)
{
    env["SERVER_PROTOCOL"] = request.getMethodProtocol();
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_SOFTWARE"] = "Webserv/1.0";
    env["METHOD"] = request.getMethodName();
    env["PATH_INFO"] = locationPath;
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
    LocationFinder locationFinder;
    locationFinder.locationMatch(&client, requestBuffer, nSocket);
    std::string rootFolder = locationFinder._root;
    std::string location = locationFinder._locationPath;
    std::string index = locationFinder._index;
    locationPath = locationFinder._pathToServe;

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
    
    proccessCGI(&client, nSocket, request);
}

//--- Main function to process CGI
void HandleCgi::proccessCGI(Client* client, int nSocket, Request &request)
{
    int pipe_in[2];
    int pipe_out[2];
    
    if (pipe(pipe_out) == -1 || pipe(pipe_in) == -1)
        throw std::runtime_error("500");
    
    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("500");
    else if (pid == 0)
        handleChildProcess(pipe_in, pipe_out, locationPath, request);
    else
        handleParentProcess(client, nSocket, pipe_in, pipe_out, pid, request);
}

//-- Function to determine the executable based on the file extension
std::string HandleCgi::getExecutable(const std::string &locationPath)
{
    size_t pos = locationPath.rfind(".");
    std::string extension;
    if (pos != std::string::npos)
        extension = locationPath.substr(pos);
    else
        throw std::runtime_error("403"); //-- NOT SURE IF 403 IS THE RIGHT ERROR CODE

    std::string executable;
    executable = Helper::executableMap.find(extension)->second;
    if (executable.empty())
        throw std::runtime_error("403"); //-- NOT SURE IF 403 IS THE RIGHT ERROR CODE

    return executable;
}

//-- Function to handle the child process
void HandleCgi::handleChildProcess(int pipe_in[2], int pipe_out[2], const std::string &locationPath, Request &request)
{
    dup2(pipe_in[0], STDIN_FILENO); //-- Redirect stdin to read end of the pipe
    dup2(pipe_out[1], STDOUT_FILENO); //-- Redirect stdout to write end of the pipe

    close(pipe_in[0]); //-- Close write end of the pipe
    close(pipe_out[1]); //-- Close read end of the pipe
    
    initEnv(request);
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
    std::cout << BOLD RED << "EXECVE FAILED" << RESET << std::endl;
    throw std::runtime_error("500");
}

//----- Function to handle the parent process
//--- NOTE: This should go through epoll
//--- read there from pipe_fd[0] and write to the client socket
void HandleCgi::handleParentProcess(Client* client, int nSocket, int pipe_in[2], int pipe_out[2], pid_t pid, Request &request)
{
    close(pipe_in[0]); //-- Close read end of the pipe
    close(pipe_out[1]); //-- Close write end of the pipe

    std::vector<char> response(request._requestBody.begin(), request._requestBody.end());

    // std::cout << BOLD YELLOW << "REQUEST BODY: " << request._requestBody << RESET << std::endl;

    //-- Write the body to the CGI script's stdin
    // Write the body to the CGI script's stdin
    ssize_t bytes_written = write(pipe_in[1], response.data(), response.size());
    if (bytes_written == -1)
    {
        close(pipe_in[1]);
        throw std::runtime_error("500");
    }
    // std::cout << "Bytes written to pipe: " << bytes_written << std::endl;
    // std::cout << "Data written to pipe: " << std::string(response.begin(), response.end()) << std::endl;

    
    close(pipe_in[1]); //-- Close write end after sending data

    //--- Read CGI output from the pipe

    std::ostringstream bodyStr;
    int status;
    std::vector<char> cgiOutput(64000); //-- Buffer size of 64 KB
    while (true)
    {
        ssize_t n = read(pipe_out[0], cgiOutput.data(), cgiOutput.size());
        if (n > 0)
            bodyStr.write(cgiOutput.data(), n);
        else if (n == 0)
            break;
        else
        {
            close(pipe_out[0]);
            throw std::runtime_error("500");
        }

        //-- Check if the child process has exited
        //-- WNOHANG: return immediately if no child has exited
        if (waitpid(pid, &status, WNOHANG) == pid)
        {
            if (WIFEXITED(status))
                std::cout << BOLD BLUE << "Process completed with exit code: " << WEXITSTATUS(status) << RESET << std::endl;
            break;
        }
    }

    close(pipe_out[0]); //-- Close read end after reading data
    std::string body = bodyStr.str();

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
    size_t bodyStart = body.find("\r\n\r\n");
    if (bodyStart != std::string::npos)
    {
        bodyStart += 5;
        body.erase(0, bodyStart);
    }
    std::string bodyNoheader(body.data(), body.size());
    request.setMethodMimeType(setMime);
    request._response->createHeaderAndBodyString(request, bodyNoheader, "200", client);
    (void) nSocket; // BP: remove this
}

HandleCgi::~HandleCgi()
{
    env.clear();
}
