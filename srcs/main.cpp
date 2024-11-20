
#include "../includes/main.hpp"
#include "../includes/ServerManager.hpp"

#include <csignal>
#include <exception>

volatile sig_atomic_t stopSignal = 0;

void signalHandler(int signal) {
	if (signal == SIGINT) {
		stopSignal = 1;
		std::cout << std::endl;
	}
}

int main(int argc, char **argv)
{
	signal(SIGINT, signalHandler);
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
		std::cerr << BOLD RED "\nError:\t" << e.what() << RESET << std::endl;
	}
	serverManager.shutdown();

}
