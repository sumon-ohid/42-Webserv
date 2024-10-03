#include <csignal>

#include "Socket.hpp"
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
	std::cout << "Server started. Listening at Port " << PORT << std::endl;
	Socket	socket(PORT);
	try
	{
		ServerConfig config(argv[1]);
		//config.displayConfig();
		socket.createSocket();
	}
	catch (std::exception& e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
}
