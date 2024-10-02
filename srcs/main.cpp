#include "Server.hpp"
#include "Config.hpp"
#include "main.hpp"

int main(int argc, char **argv)
{
	// Config config(argv[1]);
	// config.printConfig();
	(void) argv;
	if (argc > 2) {
		std::cerr << "Wrong use of webserv!\nCorrect use: ./webserv configuration-file" << std::endl;
		return ERROR;
	}
	Server	server;
	// Socket	socket(PORT);
	try
	{
		// socket.createSocket();
		server.createSockets();
		server.callEpoll();
	}
	catch (std::exception &e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
		
	}
}
