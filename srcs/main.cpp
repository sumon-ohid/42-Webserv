#include <iostream>

#include "main.hpp"

int main(int argc, char **argv) {
	if (argc > 2) {
		std::cerr << "Wrong unse of webserv!\nCorrect use: ./webserv configuration-file" << std::endl;
		return ERROR;
	}
	std::cout << "Server started. Listening at Port " << PORT << std::endl;
}