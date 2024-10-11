//-- Written by : msumon

#include "HandleCgi.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"

HandleCgi::HandleCgi()
{
    cgiPath = "./cgi-bin/";
    cgiConf = "Default";
}

//-- Constructor to handle the CGI request
//-- I will parse the request to get the path of the CGI file
//-- then I will call the proccessCGI function to execute the CGI script
//-- and send the output to the client
HandleCgi::HandleCgi(std::string requestBuffer, int nSocket, Server* server, std::string locationPath)
{
    (void)server;
    std::string clientMessage(requestBuffer.begin(), requestBuffer.end());

    size_t pos = clientMessage.find("/cgi-bin/");
    if (pos != std::string::npos)
    {
        cgiPath = requestBuffer;

        cgiConf = locationPath;

        std::cout << "++++++ >> " << cgiConf << std::endl;
        if (cgiConf != requestBuffer)
            throw std::runtime_error("404");
        proccessCGI(nSocket);
    }
}

void HandleCgi::proccessCGI(int nSocket)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
        throw std::runtime_error("Pipe failed !!");

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("Fork failed !!");
    else if (pid == 0)
    {
        //--- Child process
        //--- Redirect stdout to the pipe's write end
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]); //--- Close unused read end
        close(pipe_fd[1]); //--- Close write end after redirection

        //--- Determine the executable based on the file extension
        size_t pos = cgiPath.find(".");
        std::string extension = cgiPath.substr(pos);
        std::string exe;

        std::string fullPath = "." + cgiPath;
        if (extension == ".py")
            exe = "/usr/bin/python3";
        else if (extension == ".php")
            exe = "/usr/bin/php";
        else if (extension == ".sh")
            exe = "/usr/bin/bash";
        else
            throw std::runtime_error("This file extension is not supported !!");

        //--- Prepare arguments for execve
        char *const argv[] = { (char *)exe.c_str(), (char *)fullPath.c_str(), NULL };
        char *const envp[] = { NULL };

        execve(exe.c_str(), argv, envp);
        throw std::runtime_error("Execve failed !!");
    }
    else
    {
        //--- Parent process
        close(pipe_fd[1]); //--- Close unused write end

        //--- Read CGI output from the pipe
        std::vector<char> cgiOutput(1024);
        ssize_t n = read(pipe_fd[0], cgiOutput.data(), cgiOutput.size());
        if (n < 0)
            throw std::runtime_error("Read failed !!");

        //--- Resize the vector to the actual size of the read data
        cgiOutput.resize(n);

        //--- Prepare HTTP response request
        std::string httpRequest = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " + to_string(n) + "\n\n";

        //--- Send the HTTP request
        send(nSocket, httpRequest.c_str(), httpRequest.size(), 0);

        //--- Send the CGI output
        send(nSocket, cgiOutput.data(), cgiOutput.size(), 0);

        close(pipe_fd[0]); //--- Close read end
        waitpid(pid, NULL, 0); //--- Wait for the child process to finish
    }
}

HandleCgi::~HandleCgi()
{
}
