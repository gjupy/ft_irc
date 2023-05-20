#include "../includes/Client.hpp"

int Client::_unregistered_count = 0;

Client::Client(int fd, Server &server, size_t poll_fds_arr_pos)
    : _fd(fd), _server(server), _poll_fds_arr_pos(poll_fds_arr_pos),
      _is_registered(false), _user(""), _is_authenticated(false), _buffer("") {
  _unregistered_count++;
  std::ostringstream oss;
  oss << "unregistered_" << _unregistered_count;
  _nick = oss.str();

  _commands["PASS"] = &Client::handle_pass;
  _commands["NICK"] = &Client::handle_nick;
  _commands["USER"] = &Client::handle_user;
  _commands["JOIN"] = &Client::handle_join;
  _commands["KICK"] = &Client::handle_kick;
  _commands["TOPIC"] = &Client::handle_topic;
  _commands["INVITE"] = &Client::handle_invite;
  _commands["MODE"] = &Client::handle_mode;
  _commands["PRIVMSG"] = &Client::handle_privmsg;
  _commands["CAP"] = &Client::handle_capabilities;
  _commands["PING"] = &Client::handle_ping;
  _commands["QUIT"] = &Client::handle_quit;
}

Client::~Client() {
  _unregistered_count--;
  std::map<std::string, Channel *> channels = _server.get_channels();
  for (map_channels::const_iterator it = channels.begin(); it != channels.end();
       ++it) {
    if (is_registered_channel(it->second))
      leave_channel(it->first);
  }
}

Client &Client::operator=(const Client &rhs) {
  _fd = rhs._fd;
  _is_registered = rhs._is_registered;
  _nick = rhs._nick;
  _user = rhs._user;
  _is_authenticated = rhs._is_authenticated;
  _buffer = rhs._buffer;
  _server = rhs._server;
  _poll_fds_arr_pos = rhs._poll_fds_arr_pos;
  return (*this);
}

Client::Client(const Client &src) : _server(src._server) { *this = src; }

const std::string &Client::get_nickname() const { return _nick; }

const std::string &Client::get_username() const { return _user; }

std::string &Client::get_buffer() { return _buffer; }

bool Client::is_valid_key(Channel *channel, const std::string &input_key) {
  if (channel->get_key() == input_key)
    return (true);
  return (false);
}

bool Client::is_invited(const Channel *channel, const std::string &nickname) {
  client_set invited = channel->get_invited();

  for (client_set::iterator it = invited.begin(); it != invited.end(); ++it) {
    if ((*it)->_nick == nickname)
      return (true);
  }
  return (false);
}

bool Client::is_registered_channel(const Channel *channel) {
  client_set registered = channel->get_registered();

  for (client_set::iterator it = registered.begin(); it != registered.end();
       ++it) {
    if ((*it)->_nick == _nick)
      return (true);
  }
  return (false);
}

bool are_remain_args(std::istringstream &iss) {
  std::string other;

  iss >> other;
  if (!other.empty())
    return (true);
  return (false);
}

void Client::handle_capabilities(const std::string &buffer) { (void)buffer; }

void Client::handle_ping(const std::string &buffer) {
  _server.send_to_client(_nick, std::string(":") + SERVER + " PONG " + SERVER +
                                    " :" + buffer + "\r\n");
}

void Client::leave_channel(const std::string &channel_name) {
  Channel *channel = find_channel(_server.get_channels(), channel_name);
  if (channel == NULL)
    throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                " " + channel_name + " : No such channel");
  if (!is_member(channel->get_registered(), _nick))
    throw std::invalid_argument(std::string(":") + "441 " + SERVER + " " +
                                _nick + " " + channel_name +
                                " : is not on channel ");
  send_to_channel(*channel, std::string(":") + _nick + "!" + _user + "@" +
                                HOST + " QUIT :Goodbye!\r\n");
  channel->erase_user(_nick);
  if (channel->get_registered().size() == 0)
    _server.erase_channel(channel_name);
}

void Client::add_user(map_str_str &channels_to_keys) {
  map_channels channels = _server.get_channels();
  for (map_str_str::iterator it = channels_to_keys.begin();
       it != channels_to_keys.end(); ++it) {
    Channel *channel = find_channel(channels, it->first);
    if (channel == NULL) {
      channel = new Channel(it->first, it->second, _nick);
      _server.add_new_channel(channel);
    } else if (is_registered_channel(channel))
      continue;
    else if (channel->get_user_limit() &&
             channel->get_registered().size() >= channel->get_user_limit())
      throw std::invalid_argument(
          std::string(":") + SERVER + " 471 " + _nick + " " + it->first +
          " : Cannot join channel (+l) - channel is full");
    else if (channel->get_invite_only() && !is_invited(channel, _nick))
      throw std::invalid_argument(
          std::string(":") + SERVER + " 473 " + _nick + " " + it->first +
          " : Cannot join channel (+i) - you must be invited");
    else if (channel->get_key_needed() && !is_valid_key(channel, it->second))
      throw std::invalid_argument(std::string(":") + SERVER + " 475 " + _nick +
                                  " " + it->first +
                                  " : Cannot join channel (+k) - bad key");
    channel->set_registered(*this);
    send_to_channel(*channel, std::string(":") + _nick + "!" + _user + "@" +
                                  HOST + " JOIN " + it->first + " * :" + _user +
                                  "\r\n");
    _server.send_to_client(_nick, std::string(":") + _nick + "!" + _user + "@" +
                                      HOST + " JOIN " + it->first +
                                      " * :" + _user + "\r\n");
    _server.send_to_client(_nick, std::string(":") + SERVER + " 332 " + _nick +
                                      " " + it->first + " :" +
                                      channel->get_topic() + "\r\n");
    _server.send_to_client(_nick, std::string(":") + SERVER + " 353 " + _nick +
                                      " = " + it->first + " :" +
                                      channel->get_registered_list() + "\r\n");
    _server.send_to_client(_nick, std::string(":") + SERVER + " 366 " + _nick +
                                      " " + it->first +
                                      " :END of NAMES list\r\n");
  }
}

void Client::send_to_channel(const Channel &channel,
                             const std::string &message) {

  client_set registered = channel.get_registered();
  for (client_set::iterator it = registered.begin(); it != registered.end();
       ++it) {
    if ((*it)->get_nickname() != _nick)
      _server.send_to_client((*it)->get_nickname(), message);
  }
}

void Client::join_parser(const std::string &buffer,
                         map_str_str &channels_to_keys) {
  std::string channel_list, key_list;
  std::istringstream iss(buffer);
  iss >> channel_list >> key_list;

  if (channel_list.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 JOIN :Not enough parameters");
  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 JOIN :Too many parameters");
  std::istringstream channel_stream(channel_list);
  std::istringstream key_stream(key_list);
  std::string channel, key;
  while (std::getline(channel_stream, channel, ',')) {
    if (!std::getline(key_stream, key, ','))
      key = "";
    channels_to_keys[channel] = key;
  }
  for (map_str_str::iterator it = channels_to_keys.begin();
       it != channels_to_keys.end(); ++it) {
    if (it->first[0] != '#' || it->first.length() == 1 ||
        it->first.find(' ') != std::string::npos ||
        it->first.find(',') != std::string::npos ||
        it->first.find((char)7) != std::string::npos || it->first.length() > 50)
      throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                  " " + it->first +
                                  " : Erroneous channel name");
  }
  try {
    add_user(channels_to_keys);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument(e.what());
  }
}

/*
c1 *(,cn) *(key) *(, key)
keys are assigned to the channels in the order they are written
*/
void Client::handle_join(const std::string &buffer) {
  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered");

  map_str_str channels_to_keys;
  try {
    join_parser(buffer, channels_to_keys);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument(e.what());
  }
}

void Client::parse_command(const std::string &command) {
  std::istringstream iss(command);
  std::string cmd;
  iss >> cmd;

  std::cout << "command: " << command << std::endl;
  for (std::map<std::string, CommandHandler>::iterator it = _commands.begin();
       it != _commands.end(); ++it) {
    if (cmd == it->first) {

      std::string args;
      std::getline(iss, args); // Get the entire remaining input line
      if (!args.empty() && args[0] == ' ')
        args.erase(0, 1); // Remove the leading space
      try {
        (this->*(it->second))(args);
        return;
      } catch (const std::exception &e) {
        _server.send_to_client(_nick, e.what() + std::string("\r\n"));
        return;
      }
    }
  }
  std::cerr << "Error\n"
            << "invalid input\n";
  _server.send_to_client(_nick, std::string(":") + SERVER + " 421 " + _nick +
                                    " " + cmd + " :Unknown command\r\n");
}

void Client::handle_pass(const std::string &args) {
  if (_is_authenticated)
    throw std::invalid_argument(std::string(":") + SERVER + " 462 " + _nick +
                                " :You may not reregister");
  if (args.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 PASS :Not enough parameters");
  std::string arg;
  std::istringstream iss(args);
  iss >> arg;

  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 PASS :Too many parameters");
  if (arg != _server.get_password())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 464 PASS :Password incorrect");

  _is_authenticated = true;
  std::cout << "Client " << _fd << " authenticated" << std::endl;
}

bool Client::username_exists(const std::string &username) const {
  for (map_clients::const_iterator it = _server.get_clients().begin();
       it != _server.get_clients().end(); ++it) {
    if (it->second->get_username() == username)
      return (true);
  }
  return (false);
}

void Client::handle_user(const std::string &args) {
  std::istringstream iss(args);
  std::string arg;

  if (!_is_authenticated)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not authenticated");
  if (!_user.empty())
    throw std::invalid_argument(std::string(":") + SERVER + " 462 " + _nick +
                                " :You may not reregister");
  if (args.empty() || !(iss >> arg))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 USER :Not enough parameters");
  if (username_exists(args))
    throw std::invalid_argument(std::string(":") + SERVER + " 433 " + _nick +
                                " :Nickname is already in use");
  if (arg[0] == '#' || arg == "*")
    throw std::invalid_argument(std::string(":") + SERVER + " 432 " + _nick +
                                " " + arg + " :Erroneous nickname");
  _user = arg;
  std::cout << "Client " << _fd << " set username to: " << _user << std::endl;
  if (!_nick.empty() || _nick != "*") {
    _is_registered = true;
    _server.send_to_client(_nick, std::string(":") + SERVER + " 001 " + _nick +
                                      " :Welcome to the ft_irc network " +
                                      _nick + "!" + _user + "@" +
                                      "localhost\r\n");
    std::cout << GREEN << "Client " << _nick
              << " successfully registered to the server" << RESET << std::endl;
  }
}

bool Client::nickname_exists(const std::string &nickname) const {
  for (map_clients::const_iterator it = _server.get_clients().begin();
       it != _server.get_clients().end(); ++it) {
    if (it->second->get_nickname() == nickname)
      return (true);
  }
  return (false);
}

void Client::handle_nick(const std::string &args) {
  if (!_is_authenticated)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not authenticated");
  if (!_nick.empty() && _nick.substr(0, 13) != "unregistered_")
    throw std::invalid_argument(std::string(":") + SERVER + " 462 " + _nick +
                                " :You may not reregister");

  std::string arg;
  std::istringstream iss(args);
  if (!(iss >> arg))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 NICK: Not enough parameters");
  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 NICK: Too many parameters");
  if (nickname_exists(args) || username_exists(args) || args == _user)
    throw std::invalid_argument(std::string(":") + SERVER + " 433 " + args +
                                " :Nickname is already in use");
  if (arg[0] == '#')
    throw std::invalid_argument(std::string(":") + SERVER + " 432 " + _nick +
                                " " + arg + " :Erroneous nickname");
  _nick = arg;
  if (!_user.empty()) {
    _is_registered = true;
    _server.send_to_client(_nick, std::string(":") + SERVER + " 001 " + _nick +
                                      " :Welcome to the ft_irc network " +
                                      _nick + "!" + _user + "@" +
                                      "localhost\r\n");
    std::cout << GREEN << "Client " << _nick
              << " successfully registered to the server" << RESET << std::endl;
  }
}

void Client::handle_privmsg(const std::string &args) {
  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered");

  std::string target, message;
  std::istringstream iss(args);
  if (!std::getline(iss, target, ' '))
    throw std::invalid_argument(
        std::string(":") + SERVER +
        " 461 PRIVMSG: Not enough parameters: target missing");
  if (!std::getline(iss, message))
    throw std::invalid_argument(
        std::string(":") + SERVER +
        " 461 PRIVMSG: Not enough parameters: message missing");

  Channel *channel = find_channel(_server.get_channels(), target);
  std::string response = std::string(":") + _nick + "!" + _user + "@" + HOST +
                         " PRIVMSG " + target + " :" + message + "\r\n";
  if (channel) {
    if (is_registered_channel(channel))
      send_to_channel(*channel, response);
    else
      throw std::invalid_argument(std::string(":") + SERVER + " 404 " + _nick +
                                  " " + target + " :Cannot send to channel");
  } else {
    if (!_server.send_to_client(target, response))
      throw std::invalid_argument(std::string(":") + SERVER + " 401 " + _nick +
                                  " " + target + " :No such nick/channel");
  }
}

Client *Client::find_client(const map_clients &clients,
                            const std::string &nickname) const {
  map_clients::const_iterator it_clients;

  for (it_clients = clients.begin(); it_clients != clients.end();
       ++it_clients) {
    if (nickname == it_clients->second->get_nickname())
      return (it_clients->second);
  }
  return (NULL);
}

Channel *Client::find_channel(const map_channels &channels,
                              const std::string &channel_name) const {
  map_channels::const_iterator it_channels;

  if ((it_channels = channels.find(channel_name)) != channels.end())
    return (it_channels->second);
  return (NULL);
}

bool Client::is_operator(Channel &input_channel,
                         const std::string &inviter) const {
  std::set<std::string>::const_iterator it_operators;
  const std::set<std::string> operators = input_channel.get_operators();

  if ((it_operators = operators.find(inviter)) != operators.end())
    return (true);
  return (false);
}

bool Client::is_member(const client_set &registered,
                       const std::string &nickname) const {
  client_set::const_iterator it_registered;

  for (client_set::const_iterator it = registered.begin();
       it != registered.end(); ++it) {
    if (nickname == (*it)->get_nickname())
      return (true);
  }
  return (false);
}

void Client::invite_client(Client &client, Channel &channel) {
  channel.set_invited(client);
  _server.send_to_client(_nick, std::string(":") + SERVER + " 341 " + _nick +
                                    " " + client.get_nickname() + " " +
                                    channel.get_name() + "\r\n");
  _server.send_to_client(client.get_nickname(),
                         std::string(":") + _nick + "!" + _user + "@" + HOST +
                             " INVITE " + client.get_nickname() + " :" +
                             channel.get_name() + "\r\n");
}

void Client::handle_invite(const std::string &buffer) {
  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered\r\n");

  std::istringstream iss(buffer);
  std::string nickname;
  std::string channel_name;

  iss >> nickname >> channel_name;
  if (nickname.empty() || channel_name.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 INVITE: Not enough parameters");
  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 INVITE: Too many parameters");
  const std::map<std::string, Channel *> &channels = _server.get_channels();
  Channel *input_channel = find_channel(channels, channel_name);
  if (input_channel == NULL)
    throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                " " + channel_name + " : No such channel");
  const std::map<int, Client *> &clients = _server.get_clients();
  Client *input_client = find_client(clients, nickname);
  if (input_client == NULL)
    throw std::invalid_argument(std::string(":") + SERVER + " 401 " + _nick +
                                " " + nickname + " :No such nick/channel");
  if (is_member(input_channel->get_registered(), _nick) == false)
    throw std::invalid_argument(std::string(":") + SERVER + +" 401 " + _nick +
                                " " + channel_name +
                                " :You're not on that channel ");
  if (input_channel->get_invite_only() &&
      is_operator(*input_channel, _nick) == false)
    throw std::invalid_argument(std::string(":") + SERVER + " 482 " + _nick +
                                " " + channel_name +
                                " :You're not channel operator ");
  if (is_member(input_channel->get_registered(), nickname))
    throw std::invalid_argument(std::string(":") + SERVER + " 443 " + _nick +
                                " " + channel_name +
                                " :Is already on channel ");
  if (is_invited(input_channel, nickname))
    return;
  invite_client(*input_client, *input_channel);
}

void Client::handle_mode(const std::string &buffer) {
  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered\r\n");

  const std::map<std::string, Channel *> &channels = _server.get_channels();
  std::istringstream iss(buffer);
  std::string channel_name;
  std::string mode;
  std::string mode_param;

  iss >> channel_name >> mode >> mode_param;
  if (channel_name.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 MODE: Not enough parameters");
  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 MODE: Too many parameters");
  Channel *input_channel = find_channel(channels, channel_name);
  if (input_channel == NULL)
    throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                " " + channel_name + " : No such channel");
  if (mode.empty()) {
    _server.send_to_client(_nick, std::string(":") + SERVER + " 324 " + _nick +
                                      " " + channel_name + " " +
                                      input_channel->get_modes() + "\r\n");
    return;
  }
  if (mode[0] != '+' && mode[0] != '-')
    throw std::invalid_argument(std::string(":") + SERVER + " 501 " + _nick +
                                " :Unknown MODE flags");
  if (is_operator(*input_channel, _nick) == false)
    throw std::invalid_argument(std::string(":") + SERVER + " 482 " + _nick +
                                " " + channel_name +
                                " :You're not channel operator ");
  const std::map<int, Client *> &clients = _server.get_clients();
  Client *input_client = find_client(clients, mode_param);
  std::string response;
  switch (mode[1]) {
  case ('i'):
    if (mode[0] == '+')
      input_channel->set_invite_only(true);
    else
      input_channel->set_invite_only(false);
    break;
  case ('t'):
    if (mode[0] == '+')
      input_channel->set_topic_restriciton(true);
    else
      input_channel->set_topic_restriciton(false);
    break;
  case ('k'):
    if (mode_param.empty())
      throw std::invalid_argument(std::string(":") + SERVER +
                                  " 461 MODE: Not enough parameters");
    if (mode[0] == '+')
      input_channel->set_key(mode_param);
    else
      input_channel->set_key_needed(false);
    break;
  case ('o'):
    if (mode_param.empty())
      throw std::invalid_argument(std::string(":") + SERVER +
                                  " 461 MODE: Not enough parameters");
    if (input_client == NULL)
      throw std::invalid_argument(std::string(":") + SERVER + " 401 " + _nick +
                                  " " + channel_name +
                                  " :No such nick/channel");
    if (is_member(input_channel->get_registered(), mode_param) == false)
      return;
    if (mode[0] == '+') {
      if (is_operator(*input_channel, mode_param))
        return;
      input_channel->set_operator(mode_param, give);
    } else
      input_channel->set_operator(mode_param, take);
    break;
  case ('l'):
    if (mode_param.empty())
      throw std::invalid_argument(std::string(":") + SERVER +
                                  " 461 MODE: Not enough parameters");
    if (mode[0] == '+') {
      std::stringstream ss(mode_param);
      std::stringstream ss_str_limit(mode_param);
      unsigned short limit;
      ss >> limit;
      std::string str_limit;
      ss_str_limit >> str_limit;
      if (limit <= 0)
        return;
      input_channel->set_user_limit(limit);
    } else
      input_channel->set_user_limit(0);
    break;
  default:
    throw std::invalid_argument(std::string(":") + SERVER + " 501 " + _nick +
                                " :Unknown MODE flags");
    break;
  }
  if (mode[1] == 'o')
    send_to_channel(*input_channel, std::string(":") + _nick + "!" + _user +
                                        "@" + HOST + " MODE " + channel_name +
                                        " " + mode + " " + mode_param + "\r\n");
  else
    send_to_channel(*input_channel, std::string(":") + SERVER + " 324 " +
                                        _nick + " " + channel_name + " " +
                                        mode + "\r\n");
}

void Client::kick_parser(const std::string &buffer,
                         map_str_str &channels_to_nicks) const {
  std::string channel_list, nick_list;
  std::istringstream iss(buffer);
  iss >> channel_list >> nick_list;

  if (channel_list.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 KICK: Not enough parameters");
  std::istringstream channel_stream(channel_list);
  std::istringstream nick_stream(nick_list);
  std::string channel, nick;
  while (std::getline(channel_stream, channel, ',')) {
    if (!std::getline(nick_stream, nick, ','))
      throw std::invalid_argument(std::string(":") + SERVER +
                                  " 461 KICK: Not enough parameters");
    channels_to_nicks[channel] = nick;
  }
  if (std::getline(nick_stream, nick, ','))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 KICK: Not many parameters");
}

void Client::kick_user(map_str_str &channels_to_nick,
                       const map_channels &channels,
                       const map_clients &clients) {
  for (map_str_str::iterator it = channels_to_nick.begin();
       it != channels_to_nick.end(); ++it) {
    Channel *input_channel = find_channel(channels, it->first);
    if (input_channel == NULL)
      throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                  " " + it->first + " : No such channel");
    Client *input_client = find_client(clients, it->second);
    if (input_client == NULL)
      throw std::invalid_argument(std::string(":") + SERVER " 401 " + _nick +
                                  " " + it->first + " :No such nick/channel");
    if (!is_member(input_channel->get_registered(), it->second))
      throw std::invalid_argument(std::string(":") + "441 " + SERVER + " " +
                                  _nick + " " + it->first +
                                  " : is not on channel");
    if (!is_operator(*input_channel, _nick))
      throw std::invalid_argument(std::string(":") + SERVER + " 482 " + _nick +
                                  " " + it->first +
                                  " :You're not channel operator");
    if (_nick == it->second)
      _server.send_to_client(_nick, std::string(":") + _nick + "!" + _user +
                                        "@" + HOST + " KICK " + it->first +
                                        " " + it->second + " :" + "\r\n");
    send_to_channel(*input_channel, std::string(":") + _nick + "!" + _user +
                                        "@" + HOST + " KICK " + it->first +
                                        " " + it->second + " :" + "\r\n");
    input_channel->erase_user(it->second);
    if (input_channel->get_registered().size() == 0)
      _server.erase_channel(it->first);
  }
}

void Client::handle_kick(const std::string &buffer) {
  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered\r\n");

  map_str_str channels_to_nicks;
  try {
    kick_parser(buffer, channels_to_nicks);
  } catch (const std::invalid_argument &e) {
    throw std::invalid_argument(e.what());
  }
  const std::map<std::string, Channel *> &channels = _server.get_channels();
  const std::map<int, Client *> &clients = _server.get_clients();
  kick_user(channels_to_nicks, channels, clients);
}

void Client::handle_topic(const std::string &buffer) {
  std::string channel_name, topic_name;
  std::istringstream iss(buffer);
  iss >> channel_name >> topic_name;

  if (!_is_registered)
    throw std::invalid_argument(std::string(":") + SERVER + " 451 " +
                                ":You have not registered");
  if (are_remain_args(iss))
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 TOPIC: Too many parameters");
  if (channel_name.empty())
    throw std::invalid_argument(std::string(":") + SERVER +
                                " 461 TOPIC: Not enough parameters");
  const std::map<std::string, Channel *> &channels = _server.get_channels();
  Channel *input_channel = find_channel(channels, channel_name);
  if (input_channel == NULL)
    throw std::invalid_argument(std::string(":") + SERVER + " 403 " + _nick +
                                " " + channel_name + " : No such channel");
  if (topic_name.empty()) {
    const std::string &topic = input_channel->get_topic();
    if (topic == "")
      _server.send_to_client(_nick, std::string(":") + SERVER + " 331 " +
                                        _nick + " " + channel_name +
                                        " :No topic is set\r\n");
    else
      _server.send_to_client(_nick, std::string(":") + SERVER + " 332 " +
                                        _nick + " " + channel_name + " :" +
                                        topic + "\r\n");
  } else if (input_channel->get_topic_restriciton() &&
             !is_operator(*input_channel, _nick))
    throw std::invalid_argument(std::string(":") + SERVER + " 482 " + _nick +
                                " " + channel_name +
                                " :You're not channel operator");
  else {
    send_to_channel(*input_channel, std::string(":") + _nick + "!" + _user +
                                        "@" + HOST + " TOPIC " + channel_name +
                                        " :" + topic_name + "\r\n");
    input_channel->set_topic(topic_name);
  }
}

void Client::handle_quit(const std::string &buffer) {
  (void)buffer;
  std::map<std::string, Channel *> channels = _server.get_channels();
  for (map_channels::const_iterator it = channels.begin(); it != channels.end();
       ++it) {
    if (is_registered_channel(it->second))
      leave_channel(it->first);
  }
  _server.handle_client_disconnection(_poll_fds_arr_pos);
}
