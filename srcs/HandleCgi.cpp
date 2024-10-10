//-- Written by : msumon

#include "HandleCgi.hpp"
#include "Server.hpp"
#include "ServerConfig.hpp"

HandleCgi::HandleCgi()
{
    cgiPath = "./cgi-bin/";
    cgiConf = "Default";
}

//-- Function to get the path of the CGI from configuration file
//-- then I will compare it with the path from the request
//-- if it matches, I will return the path of the CGI file
//-- else I will throw an exception and show 404 not found


//-- Constructor to handle the CGI request
//-- I will parse the request to get the path of the CGI file
//-- then I will call the proccessCGI function to execute the CGI script
//-- and send the output to the client
HandleCgi::HandleCgi(std::string requestBuffer, int nSocket, Server* server)
{
    std::string clientMessage(requestBuffer.begin(), requestBuffer.end());

    size_t pos = clientMessage.find("/cgi-bin/");
    if (pos != std::string::npos)
    {
        cgiPath = clientMessage.substr(pos + 9); //--- Extract path after "/cgi-bin/"

        try
        {
            std::string fullCgiPath = "./cgi-bin/" + cgiPath;
            ServerConfig serverConf = server->_serverConfig;
            serverConf.displayConfig();

            cgiConf = serverConf.getCgiFile();
            if (cgiConf != fullCgiPath)
                throw std::runtime_error("404 Not Found !!");
            proccessCGI(nSocket);
        }
        catch (std::exception &e)
        {
            std::cerr << "RUNTIME ERROR :: " << e.what() << std::endl;
            std::string error_response = "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n";
            char buffer[1024];
            FILE *html_file = fopen("404.html", "r");
            if (!html_file)
                throw std::runtime_error("Failed to open 404.html file !!");
            while (fgets(buffer, sizeof(buffer), html_file) != NULL)
            {
                error_response += buffer;
            }
            fclose(html_file);
            send(nSocket, error_response.c_str(), error_response.size(), 0);
        }
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

        if (extension == ".py")
            exe = "/usr/bin/python3";
        else if (extension == ".php")
            exe = "/usr/bin/php";
        else if (extension == ".sh")
            exe = "/usr/bin/bash";
        else
            throw std::runtime_error("This file extension is not supported !!");

        std::string fullCgiPath = "./cgi-bin/" + cgiPath;

        //--- Prepare arguments for execve
        char *const argv[] = { (char *)exe.c_str(), (char *)fullCgiPath.c_str(), NULL };
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
