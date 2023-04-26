#include "server.hpp"
#include <iostream>
#include <sstream>

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
		return 1;
	}

	std::istringstream iss(argv[1]);
	int port;
	iss >> port;

	std::string password(argv[2]);

	Server server(port, password);
	server.run();

	return 0;
}
