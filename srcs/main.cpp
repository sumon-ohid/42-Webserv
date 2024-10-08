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
	std::vector<Server> servers;
    try {
        //-- We can run a loop of servers, and pass server[i] to Server server(config[i]);
		if (argc == 2) 
		{
            ServerConfig config(argv[1]);
            std::vector<ServerConfig> serversConf = config.getServers();
            //config.displayConfig();
            for (size_t i = 0; i < serversConf.size(); ++i)
			{
                ServerConfig singleServerConf = serversConf[i];
                Server server(singleServerConf);
                server.setUpLstnSockets();
                server.startEpollRoutine();
                servers.push_back(server);
            }
        }
		else
		{
            std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
            return 1;
        }
    }
    catch (std::exception& e)
	{
        std::cout << "Error:\t" << e.what() << std::endl;
        return 1;
    }

    for (size_t i = 0; i < servers.size(); ++i)
	{
        servers[i].shutdownServer();
    }

    return 0;
}
