#pragma once

#define PURPLE "\033[0;35m"
#define GREEN "\033[0;32m"
#define RESET "\033[0m"
#define RED "\033[31m"
#define BLUE "\033[34m"

class Server;
class Channel;
class Client;

#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include <exception>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdio.h>
#include <string>

typedef std::map<int, Client *> map_clients;
typedef std::map<std::string, Channel *> map_channels;
typedef std::map<std::string, std::string> map_str_str;
typedef std::set<Client *> client_set;

class Client {
private:
  int m_fd;
  Server &_server;
  size_t _poll_fds_arr_pos;
  bool m_is_registered;
  std::string m_nickname;
  std::string m_username;
  bool m_authenticated;
  std::string buffer;
  static int unregistered_count;

  void handle_pass(const std::string &);
  void handle_nick(const std::string &);
  void handle_capabilities(const std::string &);
  void handle_ping(const std::string &);

  void handle_user(const std::string &);
  bool username_exists(const std::string &username) const;
  bool nickname_exists(const std::string &nickname) const;

  void handle_join(const std::string &);
  void join_parser(const std::string &, map_str_str &);
  bool is_valid_key(Channel *, const std::string &);
  bool is_invited(const Channel *, const std::string &);
  bool is_registered_channel(const Channel *);
  void add_user(map_str_str &);

  void handle_invite(const std::string &);
  Client *find_client(const map_clients &, const std::string &) const;
  Channel *find_channel(const map_channels &, const std::string &) const;
  bool is_operator(Channel &, const std::string &) const;
  bool is_member(const client_set &, const std::string &) const;
  void invite_client(Client &, Channel &);

  void handle_mode(const std::string &);

  void handle_kick(const std::string &);
  void kick_parser(const std::string &, map_str_str &) const;
  void kick_user(map_str_str &, const map_channels &, const map_clients &);

  void handle_topic(const std::string &);
  void handle_privmsg(const std::string &);
  void handle_quit(const std::string &);

  void send_to_channel(const Channel &channel, const std::string &message);

  void leave_channel(const std::string &channel_name);

  typedef void (Client::*CommandHandler)(const std::string &);
  std::map<std::string, CommandHandler> m_commands;

public:
  Client(int fd, Server &server, size_t poll_fds_arr_pos);
  Client(const Client &src);
  ~Client();
  Client &operator=(const Client &rhs);

  const std::string &get_nickname() const;
  const std::string &get_username() const;
  std::string &get_buffer();

  void parse_command(const std::string &command);
};
