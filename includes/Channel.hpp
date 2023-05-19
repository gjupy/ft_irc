#pragma once

#include "Client.hpp"
#include <exception>
#include <iostream>
#include <set>
#include <string>
#include <vector>

class Client;

enum e_op_privilege { give, take };
typedef std::set<Client *> client_set;

class Channel {

private:
  std::string _name;
  std::string _topic;
  std::string _key;
  std::set<std::string> _operators;
  client_set _invited;
  client_set _registered;

  unsigned short _user_limit;
  bool _invite_only;
  bool _topic_restriciton;
  bool _key_needed;

public:
  Channel(const std::string &, std::string &, const std::string &);
  ~Channel();
  Channel(const Channel &src);
  Channel &operator=(const Channel &rhs);

  bool get_invite_only() const;
  bool get_topic_restriciton() const;
  bool get_key_needed() const;
  bool get_privilege() const;
  unsigned short get_user_limit() const;
  const std::string get_modes();

  const client_set &get_invited() const;
  const client_set &get_registered() const;
  const std::string &get_key() const;
  const std::string &get_name() const;
  const std::string &get_topic() const;
  const std::set<std::string> get_operators() const;

  void set_invite_only(bool value);
  void set_topic_restriciton(bool value);
  void set_key_needed(bool value);
  void set_operator(const std::string &, int);
  void set_user_limit(unsigned short limit);

  void set_registered(Client &);
  void set_invited(Client &);
  void set_key(const std::string &);
  void set_topic(const std::string &);

  void erase_user(const std::string &nickname);
};
