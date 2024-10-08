#include <csignal>

#include "Server.hpp"
#include "ServerConfig.hpp"
#include "main.hpp"
#include <cstddef>
#include <exception>
#include <vector>

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
	// Socket	socket(PORT);
	std::vector<Server> servers;
	try
	{
		//-- We can run a loop of servers, and pass server[i] to Server server(config[i]);
		if (argc == 2)
		{
			ServerConfig config(argv[1]);
			//config.displayConfig();
			std::vector<ServerConfig> serversConf = config.getServers();
			for (size_t i = 0; i < serversConf.size(); ++i)
			{
				ServerConfig singleServerConf = serversConf[i];
				Server	server(singleServerConf);
				server.setUpLstnSockets();
				server.startEpollRoutine();
				servers.push_back(server);
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
	for (size_t i = 0; 0 < servers.size(); ++i)
		servers[i].shutdownServer();
}
