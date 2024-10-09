#include <csignal>

#include "Server.hpp"
#include "main.hpp"
#include <exception>
#include "ServerManager.hpp"

volatile sig_atomic_t stopSignal = 0;

void signalHandler(int signal) {
	std::cout << "Got a signal" << std::endl;
	if (signal == SIGINT) {
		std::cout << "Signal was SIGINT" << std::endl;
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
	ServerManager	serverManager;

	// Socket	socket(PORT);
	try
	{
		serverManager.setUp(argc, argv);
		serverManager.runRoutine();

	}
	catch (std::exception& e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
	serverManager.shutdown();
}
