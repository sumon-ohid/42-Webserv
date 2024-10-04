#include "HandleCgi.hpp"
#include "ServerConfig.hpp"
#include <cstddef>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

HandleCgi::HandleCgi()
{
    cgiPath = "./cgi-bin/";
}

HandleCgi::HandleCgi(std::vector<char> requestBuffer, int nSocket)
{
    std::vector<ServerConfig> servers = getServers();

    std::string clientMessage(requestBuffer.begin(), requestBuffer.end());
    size_t pos = clientMessage.find("/cgi-bin/");
    if (pos != clientMessage.npos)
    {
        size_t newPos = clientMessage.find("HTTP");
        std::string tempPath = clientMessage.substr(pos, newPos);
        cgiPath = tempPath.substr(9);
        proccessCGI(nSocket);
    }
}

void HandleCgi::proccessCGI(int nSocket)
{
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
        throw std::runtime_error ("Pipe failed !!");

    pid_t pid = fork();
    if (pid < 0)
        throw std::runtime_error("Forki failed !!");
    else if (pid == 0)
    {
        //--- Redirect stdout to the pipe's write end
        //--- Close unused read end
        //--- Close write end after redirection
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[0]); 
        close(pipe_fd[1]);
        size_t pos = cgiPath.find(".");

        //-- for now hard coded. Later will be read from config file
        std::string exe = cgiPath.substr(pos);
        if (exe == ".py")
            exe = "/usr/bin/python3";
        else if (exe == ".php")
            exe = "/usr/bin/php";
        else if (exe == ".sh")
            exe = "/usr/bin/bash";
        else
            throw std::runtime_error("Invalid CGI file !!");
        
        std::string cgiPath = "./cgi-bin/" + this->cgiPath;
        std::vector<ServerConfig> servers = getServers();
        std::string cgiFile;
        size_t i = 0;
        while (i < servers.size())
        {
            ServerConfig server = servers[i];
            cgiFile = server.getCgiFile();
            if (cgiFile.empty())
                throw std::runtime_error("CGI not configured !!");
            if (cgiFile == this->cgiPath)
                break;
            else  
                throw std::runtime_error("CGI file not found !!");
            i++;
        }
        std::cout << "CGI FILE : " << cgiFile << std::endl;
        execve(exe.c_str(), (char *[]) {(char *)exe.c_str(), (char *)cgiPath.c_str(), NULL}, NULL);
        throw std::runtime_error("Execve failed !!");
    }
    else
    {
        //--- parent process
        //--- Close unused write end
        //--- Close read end after reading
        close(pipe_fd[1]);

        std::vector<char> cgiOutput;
        ssize_t n = read(pipe_fd[0], &cgiOutput[0], cgiOutput.size());
        if (n < 0)
            throw std::runtime_error("Read failed !!");
        else  
        {
            cgiOutput[n] = '\0';
            std::string httpHeader = "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " + to_string(n) + "\n\n";

            //--- Send the HTTP header
            send(nSocket, httpHeader.c_str(), httpHeader.size(), 0);

            //--- Send the CGI output
            send(nSocket, &cgiOutput[0], n, 0);
        }

        close(pipe_fd[0]);
        waitpid(pid, NULL, 0);
    }    
}

HandleCgi::~HandleCgi()
{
}
