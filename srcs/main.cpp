#include "../includes/Server.hpp"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
    return 1;
  }

  std::istringstream iss(argv[1]);
  int port;
  iss >> port;
  if (iss.fail() || port < 0 || port > 65535) {
    std::cerr << "Invalid port number: " << argv[1] << std::endl;
    return 1;
  }

  std::string password(argv[2]);
  if (password.empty()) {
    std::cerr << "Invalid password: " << argv[2] << std::endl;
    return 1;
  }

  Server server(port, password);
  server.run();

  return (0);
}
