#include "../includes/Server.hpp"

void input_parser(int argc, char *argv[], int &port, std::string &password) {
  if (argc != 3)
    throw std::invalid_argument("Usage: " + std::string(argv[0]) +
                                " <port> <password>");

  std::istringstream iss(argv[1]);
  iss >> port;
  if (iss.fail() || port < 0 || port > 65535)
    throw std::invalid_argument("Invalid port number: " + std::string(argv[1]));
  password = argv[2];
  if (password.empty())
    throw std::invalid_argument("Invalid password: empty");
}

int main(int argc, char *argv[]) {
  int port;
  std::string password;
  try {
    input_parser(argc, argv, port, password);
  } catch (const std::invalid_argument &e) {
    std::cerr << e.what() << '\n';
    return (1);
  }

  Server server(port, password);
  try {
    server.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return (1);
  }

  return (0);
}
