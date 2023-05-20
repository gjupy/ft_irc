#pragma once

#include "Channel.hpp"
#include "Client.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// Names the Host for response-generation
#define HOST "localhost"

// Names the Server for response-generation
#define SERVER "irc_trash"

class Client;
class Channel;

typedef std::map<int, Client *> map_clients;
typedef std::map<std::string, Channel *> map_channels;

class Server {

private:
  int _port;
  std::string _server_pass;
  std::vector<pollfd> _poll_fds;
  map_clients _clients;
  map_channels _channels;

  /* handling connections */
  int set_nonblocking(int sockfd);
  void accept_client(int server_fd, size_t i);
  void handle_client_data(size_t i);
  void handle_client_recv_error(size_t i);
  int prepare_socket(int &, struct sockaddr_in &);
  void initialize_poll_fd(pollfd &, int &);
  std::string _err_msg;

public:
  Server(int port, const std::string &password);
  Server(const Server &src);
  Server &operator=(const Server &rhs);
  ~Server();

  void run();
  void handle_client_disconnection(size_t i);
  bool send_to_client(const std::string &target_nick,
                      const std::string &message);
  void add_new_channel(Channel *);
  void erase_channel(const std::string &);

  /* getters */
  const std::string get_password() const;
  const map_channels &get_channels() const;
  const map_clients &get_clients() const;
  const std::vector<pollfd> &get_poll_fds() const;
};
