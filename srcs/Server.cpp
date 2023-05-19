#include "../includes/Server.hpp"

/*
This file implements the Server class, which is responsible for setting up the
listening socket, accepting client connections, and handling client messages. It
uses the poll() function for asynchronous I/O to manage multiple client
connections.
*/

bool running;

Server::Server(int port, const std::string &password)
    : m_port(port), m_password(password) {}

Server::~Server() {
  // Clean up
  for (size_t i = 0; i < m_poll_fds.size(); ++i)
    close(m_poll_fds[i].fd);
  for (size_t i = 0; i < m_poll_fds.size(); ++i) {
    delete m_clients[m_poll_fds[i].fd];
    m_clients.erase(m_poll_fds[i].fd);
    close(m_poll_fds[i].fd);
  }
  for (map_channels::iterator it = m_channel.begin(); it != m_channel.end();
       it++)
    delete it->second;
  m_channel.clear();
}

void Server::erase_channel(const std::string &name) {
  for (map_channels::iterator it = m_channel.begin(); it != m_channel.end();
       it++) {
    if (it->first == name) {
      m_channel.erase(it);
      delete it->second;
      return;
    }
  }
}

Server &Server::operator=(const Server &rhs) {
  m_port = rhs.m_port;
  m_password = rhs.m_password;
  m_poll_fds = rhs.m_poll_fds;
  m_clients = rhs.m_clients;
  m_channel = rhs.m_channel;
  return (*this);
}

Server::Server(const Server &src) { *this = src; }

int Server::set_nonblocking(int sockfd) {
  if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
    std::cerr << "Error: Unable to set socket to non-blocking mode."
              << std::endl;
    return -1;
  }
  return 0;
}

const std::vector<pollfd> &Server::get_poll_fds() const { return (m_poll_fds); }

void Server::accept_client(int server_fd) {
  sockaddr_in client_addr;
  socklen_t client_addr_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (sockaddr *)&client_addr, &client_addr_len);

  if (client_fd == -1) {
    std::cerr << "Error: Failed to accept client." << std::endl;
  } else {
    std::cout << "Accepted client: " << client_fd << std::endl;

    // Set client socket to non-blocking mode
    if (set_nonblocking(client_fd) == -1) {
      std::cerr << "Error: Failed to set client socket to non-blocking mode."
                << std::endl;
      close(client_fd);
    } else {
      pollfd client_pollfd;
      client_pollfd.fd = client_fd;
      client_pollfd.events = POLLIN;
      client_pollfd.revents = 0;

      m_poll_fds.push_back(client_pollfd);

      m_clients[client_fd] = new Client(
          client_fd, *this); // Use the 'new' keyword to create a Client object
    }
  }
}

void Server::handle_client_data(size_t i) {
  char buf[1024];
  ssize_t recv_len = recv(m_poll_fds[i].fd, buf, sizeof(buf) - 1, 0);
  buf[recv_len] = '\0';

  if (recv_len > 0) {
    std::string &buffer = m_clients[m_poll_fds[i].fd]->get_buffer();
    buffer.append(buf);

    // Find the newline character
    size_t newline_pos;
    while ((newline_pos = buffer.find('\n')) != std::string::npos) {
      // Extract command and remove it from the buffer
      std::string command = buffer.substr(0, newline_pos);
      buffer.erase(0, newline_pos + 1);

      // Use Client::parse_command() to handle the command
      m_clients[m_poll_fds[i].fd]->parse_command(command);
    }
  } else if (recv_len == 0) {
    handle_client_disconnection(i);
  } else {
    handle_client_recv_error(i);
  }
}

void Server::handle_client_disconnection(size_t i) {
  int client_fd = m_poll_fds[i].fd;
  delete m_clients[client_fd];
  close(client_fd); // Close the socket file descriptor
  m_poll_fds.erase(m_poll_fds.begin() + i);
  m_clients.erase(client_fd);
  --i; // Decrement the index to account for the removed element
}

void Server::handle_client_recv_error(size_t i) {
  // No data available; non-blocking recv() returns -1 with errno set to EAGAIN
  // or EWOULDBLOCK
  if (errno != EAGAIN && errno != EWOULDBLOCK) {
    delete m_clients[m_poll_fds[i].fd];
    m_clients.erase(m_poll_fds[i].fd);
    std::cerr << "Error: Failed to receive data from client ("
              << m_poll_fds[i].fd << ")." << std::endl;
    close(m_poll_fds[i].fd);
    m_poll_fds.erase(m_poll_fds.begin() + i);
    --i; // Decrement the index to account for the removed element
  }
}

bool Server::send_to_client(const std::string &target_nickname,
                            const std::string &message) {
  for (map_clients::iterator it = m_clients.begin(); it != m_clients.end();
       ++it) {
    if (it->second->get_nickname() == target_nickname) {
      int target_fd = it->first;
      if (send(target_fd, message.c_str(), message.size(), 0) == -1) {
        std::cerr << "Error: Unable to send message to client " << target_fd
                  << std::endl;
        return false;
      }
      return true;
    }
  }
  // Target client not found
  return false;
}

void signal_handler(int sig) {
  std::cout << "Signal " << sig << " received, exiting IRC_trash ...\n";
  running = false;
}

int Server::prepare_socket(int &server_fd, struct sockaddr_in &server_addr) {
  // Create a socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    std::cerr << "Error: Unable to create socket." << std::endl;
    return -1;
  }

  // Set server socket to non-blocking mode
  if (set_nonblocking(server_fd) == -1) {
    close(server_fd);
    return -1;
  }

  // Prepare the sockaddr_in structure
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(m_port);

  // Bind the socket
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    std::cerr << "Error: Unable to bind socket." << std::endl;
    close(server_fd);
    return -1;
  }

  // Start listening
  if (listen(server_fd, 10) == -1) {
    std::cerr << "Error: Unable to listen on socket." << std::endl;
    close(server_fd);
    return -1;
  }
  return 0;
}

void Server::initialize_poll_fd(pollfd &server_pollfd, int &server_fd) {
  server_pollfd.fd = server_fd;
  server_pollfd.events = POLLIN;
  server_pollfd.revents = 0;
  m_poll_fds.push_back(server_pollfd);
}

void Server::run() {
  int server_fd;
  struct sockaddr_in server_addr;

  if (prepare_socket(server_fd, server_addr) == -1)
    return;
  std::cout << "Server listening on port " << m_port << std::endl;
  // Initialize pollfd structures for server and clients
  pollfd server_pollfd;
  initialize_poll_fd(server_pollfd, server_fd);
  signal(SIGINT, signal_handler);
  running = true;
  while (running) {
    if (poll(&m_poll_fds[0], m_poll_fds.size(), -1) == -1)
      break;
    for (size_t i = 0; i < m_poll_fds.size(); ++i) {
      if (m_poll_fds[i].revents & POLLIN) {
        if (m_poll_fds[i].fd == server_fd) {
          // New client connection
          accept_client(server_fd);
        } else {
          // Client data available
          handle_client_data(i);
        }
      }
    }
  }
}

const std::string Server::get_password() const { return (m_password); }

const map_channels &Server::get_channels() const { return (m_channel); }

void Server::add_new_channel(Channel *new_channel) {
  m_channel[new_channel->get_name()] = new_channel;
}

const std::map<int, Client *> &Server::get_clients() const {
  return (m_clients);
}
