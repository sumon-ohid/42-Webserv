#include "Socket.hpp"
#include "main.hpp"

int main(int argc, char **argv) {
	(void)argv;
	if (argc > 2) {
		std::cerr << "Wrong unse of webserv!\nCorrect use: ./webserv configuration-file" << std::endl;
		return ERROR;
	}
	std::cout << "Server started. Listening at Port " << PORT << std::endl;
	Socket	socket(PORT);
	try
	{
		socket.createSocket();
	}
	catch (std::exception e)
	{
		std::cout << "Error:\t" << e.what() << std::endl;
	}
}
