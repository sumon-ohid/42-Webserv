#include <csignal>

#include "Server.hpp"
#include "ServerConfig.hpp"
#include "main.hpp"
#include <exception>

volatile sig_atomic_t stopSignal = 0;

void signalHandler(int signal) {
	if (signal == SIGINT) {
		stopSignal = 1;
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler);
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
		if (argc == 2) {
			ServerConfig config(argv[1]);
			//config.displayConfig();
		}
		// socket.createSocket();
		server.setUpLstnSockets();	
		server.startEpollRoutine();
	}
	catch (std::exception& e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
	server.shutdownServer();
}
