#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include "Config.hpp"
#include "unistd.h"

#include "main.hpp"

int	bind_to_socket(int server_fd, sockaddr_in &address)
{
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	memset(address.sin_zero, '\0', sizeof address.sin_zero);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		std::cerr << "Error while binding to socket" << std::endl;
		return(-1);
	}
	if (listen(server_fd, 1000) < 0)
	{
		std::cerr << "Too many listening" << std::endl;
		return (-1);
	}
	return (0);
}

int	create_socket()
{
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0)
	{
		std::cerr << "Error: cannot create socket" << std::endl;
		return -1;
	}
	sockaddr_in address;
	int			addrlen = sizeof(address);
	if (bind_to_socket(server_fd, address) < 0)
		return (-1);
	int	new_socket;
	long	valread;
	std::string hello = "HTTP/1.1 200 OK\nContent-Type: text/plain\nContent-Length: 12\n\nHello world!";
	while (1)
	{
		if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
		{
			std::cerr << "Error: Accept failed" << std::endl;
			return (-1);
		}
		char buffer[30000] = {0};
        valread = read( new_socket , buffer, 30000);
		if (valread < 0)
		{
			std::cerr << "Error: Read failed" << std::endl;
			return (-1);
		}
        std::cout << buffer << "\n" << std::endl;
        write(new_socket , hello.c_str() , hello.size());
        std::cout << "------------------Hello message sent-------------------" << std::endl;;
        close(new_socket);

	}
}

int main(int argc, char **argv)
{
	Config config(argv[1]);
	config.printConfig();
	if (argc > 2) {
		std::cerr << "Wrong use of webserv!\nCorrect use: ./webserv configuration-file" << std::endl;
		return ERROR;
	}
	std::cout << "Server started. Listening at Port " << PORT << std::endl;
	create_socket();
}
