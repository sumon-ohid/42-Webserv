#include <csignal>

#include "Server.hpp"
#include "main.hpp"
#include <exception>
#include "ServerManager.hpp"

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
	if (argc > 2) {
		std::cerr << BOLD RED << "Wrong use of webserv!\nCorrect use: ./webserv configuration-file" << RESET << std::endl;
		return ERROR;
	}
	ServerManager	serverManager;
	try
	{
		serverManager.runWebservs(argc, argv);
	}
	catch (std::exception& e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
	serverManager.shutdown();
}
