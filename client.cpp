
// client.cpp
#include "client.hpp"
#include "Channel.hpp"
#include <sstream>
#include <iostream>
#include <exception>
#include <stdio.h>

int Client::unregistered_count = 0;

Client::Client(int fd, Server& server) : m_fd(fd), m_is_registered(false), _server(server), m_username(""), m_authenticated(false), buffer("")
{
	unregistered_count++;
	std::ostringstream oss;
	oss << "unregistered_" << unregistered_count;
	m_nickname = oss.str();

	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
	m_commands["JOIN"] = &Client::handle_join;
	m_commands["KICK"] = &Client::handle_kick;
	m_commands["TOPIC"] = &Client::handle_topic;
	m_commands["INVITE"] = &Client::handle_invite;
	m_commands["MODE"] = &Client::handle_mode;
	m_commands["PRIVMSG"] = &Client::handle_privmsg;
	m_commands["EXIT"] = &Client::handle_exit;
	// m_commands["PING"] = &Client::handle_ping;
	// m_commands["CAP"] = &Client::handle_cap;
	// m_commands["WHO"] = &Client::handle_who;
}

const std::string& Client::get_nickname() const {
	return m_nickname;
}

const std::string& Client::get_username() const {
	return m_username;
}

std::string& Client::get_buffer() {
	return buffer;
}

Client& Client::operator=(const Client& rhs)
{
	m_fd = rhs.m_fd;
	m_is_registered = rhs.m_is_registered;
	m_nickname = rhs.m_nickname;
	m_username = rhs.m_username;
	return (*this);
}

Client::Client(const Client& src) : _server(src._server)
{
	*this = src;
}

bool Client::is_valid_key(Channel* channel, const std::string& input_key)
{
	if (channel->get_key() == input_key)
		return (true);
	return (false);
}

bool Client::is_invited(const Channel* channel, const std::string& nickname)
{
	std::set<Client*> invited = channel->get_invited();

	for(std::set<Client*>::iterator it = invited.begin(); it != invited.end(); ++it)
	{
		if ((*it)->m_nickname == nickname)
			return (true);
	}
	return (false);
}

bool Client::is_registered_channel(const Channel* channel)
{
	std::set<Client*> registered = channel->get_registered();

	for(std::set<Client*>::iterator it = registered.begin(); it != registered.end(); ++it)
	{
		if ((*it)->m_nickname == m_nickname)
			return (true);
	}
	return (false);
}

void Client::handle_ping(const std::string& args)
{
	if (args.size() < 1) {
		// Send an error message to the client.
		_server.send_to_client(m_nickname, "461 " + m_nickname + "\r\n");
		return;
	}
	// Send a PONG response to the client.
	_server.send_to_client(m_nickname, "PONG " + m_nickname + " 127.0.0.1" + "\r\n");
}

void Client::handle_cap(const std::string& args)
{
	std::cout << "CAP: " << args << std::endl;
	// (void)args;
	return;
}

bool are_remain_args(std::istringstream& iss)
{
	std::string other;

	iss >> other;
	if (!other.empty())
		return (true);
	return (false);
}

void Client::handle_exit(const std::string& buffer)
{
	std::string							channel_name;
	std::istringstream					iss(buffer);

	iss >> channel_name;
	if (channel_name.empty() || are_remain_args(iss))
		throw std::invalid_argument("461 " + m_nickname + " EXIT: Not enough parameters");
	Channel *channel = find_channel(_server.get_channels(), channel_name);
	if (channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + ": " + channel_name + ": No such channel");
	if (!is_member(channel->get_registered(), m_nickname))
		throw std::invalid_argument("441 " + m_nickname + ": " + channel_name + ": Your aren't on that channel");
	handle_privmsg(m_nickname + " you left channel " + channel_name + "\r\n");
	handle_privmsg(channel_name + " " + m_nickname + " left channel " + channel_name + "\r\n");
	channel->erase_user(m_nickname);
	if (channel->get_registered().size() == 0)
		_server.erase_channel(channel_name);
}

void Client::add_user(std::map<std::string, std::string> &channels_to_keys)
{
	std::map<std::string, Channel*> channels = _server.get_channels();
	std::map<std::string, Channel*>::iterator it_channel;
	for (std::map<std::string, std::string>::iterator it = channels_to_keys.begin(); it != channels_to_keys.end(); ++it)
	{
		if ((it_channel = channels.find(it->first)) != channels.end()) // if channels exists
		{
			if (is_registered_channel(it_channel->second))
				return ; // USER ALREADY REGISTERED
			if (it_channel->second->get_user_limit() && it_channel->second->get_registered().size() >= it_channel->second->get_user_limit())
				throw std::invalid_argument("471 " + m_nickname + " " + it_channel->first + ": Cannot join channel (+l)");
			if (it_channel->second->get_invite_only() && !is_invited(it_channel->second, m_nickname))
				throw std::invalid_argument("473 " + m_nickname + " " + it_channel->first + ": Cannot join channel (+i)");
			if (it_channel->second->get_key_needed() && !is_valid_key(it_channel->second, it->second))
				throw std::invalid_argument("475 " + m_nickname + " " + it_channel->first + ": Cannot join channel (+k)");
			it_channel->second->set_registered(*this);
			_server.send_to_client(m_nickname, m_nickname + ": you joined channel " + it->first + "\r\n");
			handle_privmsg(it->first + " " + m_nickname + " joined channel " + it->first + "\r\n");
		}
		else
		{
			Channel* new_channel = new Channel(it->first, it->second, m_nickname);
			new_channel->set_registered(*this);
			_server.add_new_channel(new_channel);
			_server.send_to_client(m_nickname, m_nickname + ": you created channel " + it->first + "\r\n");
		}
	}
}

void Client::join_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_keys)
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	if (channel_list.empty() || are_remain_args(iss))
		// throw std::invalid_argument("invalid input format\nusage: JOIN <channel> *(\",\" <channel>) [<key> *(\",\" <key>)]");
		throw std::invalid_argument("461 " + m_nickname + " JOIN: Not enough parameters");
	std::istringstream channel_stream(channel_list);
	std::istringstream key_stream(key_list);
	std::string channel, key;
	while (std::getline(channel_stream, channel, ','))
	{
		if (!std::getline(key_stream, key, ','))
			key = "";
		channels_to_keys[channel] = key;
	}
	for (std::map<std::string, std::string>::iterator it = channels_to_keys.begin(); it != channels_to_keys.end(); ++it)
	{
		if (it->first[0] != '#' || it->first.length() == 1
			|| it->first.find(' ') != std::string::npos || it->first.find(',') != std::string::npos
			|| it->first.find((char)7) != std::string::npos
			|| it->first.length() > 50)
			throw std::invalid_argument("410 " + m_nickname + " " + it->first + ": Erroneous channel name");
	}
	try
	{
		add_user(channels_to_keys);
	}
	catch(const std::invalid_argument& e)
	{
		throw std::invalid_argument(e.what());
	}
}


/*
c1 *(,cn) *(key) *(, key)
keys are assigned to the channels in the order they are written
*/
void Client::handle_join(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please register first");

	std::map<std::string, std::string>	channels_to_keys;
	try
	{
		join_parser(buffer, channels_to_keys);
	}
	catch(const std::invalid_argument& e)
	{
		throw std::invalid_argument(e.what());
	}
}

void Client::parse_command(const std::string &command) {
	std::istringstream iss(command);
	std::string cmd;
	iss >> cmd;

	for (std::map<std::string, CommandHandler>::iterator it = m_commands.begin(); it != m_commands.end(); ++it)
	{
		if (cmd == it->first)
		{

			std::string args;
			std::getline(iss, args); // Get the entire remaining input line
			if (!args.empty() && args[0] == ' ')
				args.erase(0, 1); // Remove the leading space
			try
			{
				(this->*(it->second))(args);
				return ;
			}
			catch(const std::exception& e)
			{
				_server.send_to_client(m_nickname, e.what() + std::string("\r\n"));
				return ;
			}
		}
	}
	std::cerr << "Error\n" << "invalid input\n";
	_server.send_to_client(m_nickname, "421 " + cmd + ": Unknown command\r\n");
}

void Client::handle_pass(const std::string &args) {
	if (m_authenticated)
		throw std::invalid_argument("462 : You are already registered");

	std::string arg;
	std::istringstream iss(args);
	iss >> arg;

	if (arg != _server.get_password() || are_remain_args(iss))
		throw std::invalid_argument("464 :Password incorrect");

	m_authenticated = true;
	std::cout << "Client " << m_fd << " authenticated" << std::endl;
}

bool Client::username_exists(const std::string &username) const
{
	for (std::map<int, Client*>::const_iterator it = _server.get_clients().begin(); it != _server.get_clients().end(); ++it)
	{
		if (it->second->get_username() == username)
			return (true);
	}
	return (false);
}

void Client::handle_user(const std::string &args) {
	if (!m_authenticated)
		throw std::invalid_argument("451 : Please provide a server password using PASS command");
	if (!m_username.empty())
		throw std::invalid_argument("462 : Unauthorized command (already registered)");
	if (username_exists(args))
		throw std::invalid_argument("433 : " + args + ": This username is already in use");

	std::istringstream iss(args);
	std::string arg;

	if (args.empty() || !(iss >> arg) || are_remain_args(iss))
		throw std::invalid_argument("461 : USER: Not enough parameters");
	if (arg[0] == '#' || arg == "*")
		throw std::invalid_argument("438 " + arg + ": Erroneous username");
	m_username = arg;
	std::cout << "Client " << m_fd << " set username to: " << m_username << std::endl;
}

bool Client::nickname_exists(const std::string &nickname) const
{
	for (std::map<int, Client*>::const_iterator it = _server.get_clients().begin(); it != _server.get_clients().end(); ++it)
	{
		if (it->second->get_nickname() == nickname)
			return (true);
	}
	return (false);
}

void Client::handle_nick(const std::string &args) {
	if (!m_authenticated || m_username.empty())
		throw std::invalid_argument("451 : You need to register using PASS and USER commands");
	if (!m_nickname.empty() && m_nickname.substr(0, 13) != "unregistered_")
		throw std::invalid_argument("462 : Unauthorized command (already registered)");
	if (nickname_exists(args) || args == m_username)
		throw std::invalid_argument("433 " + args + ": Nickname is already in use");

	std::string arg;
	std::istringstream iss(args);
	if (!(iss >> arg) || are_remain_args(iss))
		throw std::invalid_argument("461 : NICK: Not enough parameters");
	if (arg[0] == '#')
		throw std::invalid_argument("432 " + arg + ": Erroneous nickname");
	m_nickname = arg;
	m_is_registered = true;
	std::cout << "Client " << m_fd << " set nickname to: " << m_nickname << std::endl;
	_server.send_to_client(m_nickname, "001 " + m_nickname + ": Welcome to the Internet Relay Network, " + m_nickname + "!" + "\r\n"); // 001: RPL_WELCOME
	_server.send_to_client(m_nickname, "002 " + m_nickname + ": Your host is " + std::string(HOST) + ", running version IRC_TRASH" + "\r\n"); // 002: RPL_YOURHOST
}

void Client::handle_privmsg(const std::string& args) {
	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please set both USER and NICK before using other commands");

	std::string target, message;
	std::istringstream iss(args);
	if (!std::getline(iss, target, ' '))
		throw std::invalid_argument("461 " + m_nickname + ": PRIVMSG: Not enough parameters: target missing");
	if (!std::getline(iss, message))
		throw std::invalid_argument("461 " + m_nickname + ": PRIVMSG: Not enough parameters: message missing");
	if (are_remain_args(iss))
		throw std::invalid_argument("461 " + m_nickname + ": PRIVMSG: Too many parameters");

	Channel *channel = find_channel(_server.get_channels(), target);
	if (channel)
	{
		const std::set<Client*>& registered_clients = channel->get_registered();
		if (is_registered_channel(channel)) {
			for (std::set<Client*>::iterator it = registered_clients.begin(); it != registered_clients.end(); ++it) {
				Client* client = *it;
				if (client->get_nickname() != m_nickname) {
					std::string response = m_nickname + ": " + target + ": " + message + "\r\n";
					_server.send_to_client(client->get_nickname(), response);
				}
			}
		} else
			throw std::invalid_argument("404 " + m_nickname + ": " + target + ": Cannot send to channel");
	}
	else
	{
		// If the target is not an existing channel, it's a private message to a user or a non-existing channel
		std::string response = m_nickname + ": PRIVMSG: " + message + "\r\n";
		if (!_server.send_to_client(target, response))
			throw std::invalid_argument("401 " + m_nickname + ": " + target + ": No such nick/channel");
	}
}

Client* Client::find_client(const std::map<int, Client*>& clients, const std::string& nickname) const
{
	std::map<int, Client*>::const_iterator it_clients;

	for (it_clients = clients.begin(); it_clients != clients.end(); ++it_clients)
	{
		if (nickname == it_clients->second->get_nickname())
			return (it_clients->second);
	}
	return (NULL);
}

Channel* Client::find_channel(const std::map<std::string, Channel*>& channels, const std::string& channel_name) const
{
	std::map<std::string, Channel*>::const_iterator it_channels;

	if ((it_channels = channels.find(channel_name)) != channels.end())
		return (it_channels->second);
	return (NULL);
}

bool Client::is_operator(Channel& input_channel, const std::string& inviter) const
{
	std::set<std::string>::const_iterator			it_operators;
	const std::set<std::string> operators = input_channel.get_operators();

	if ((it_operators = operators.find(inviter)) != operators.end())
		return (true);
	return (false);
}

bool Client::is_member(const std::set<Client*>& registered, const std::string& nickname) const
{
	std::set<Client*>::const_iterator			it_registered;

	for (std::set<Client*>::const_iterator it = registered.begin(); it != registered.end(); ++it)
	{
		if (nickname == (*it)->get_nickname())
			return (true);
	}
	return (false);
}

void Client::invite_client(Client& client, Channel& channel)
{
	channel.set_invited(client);
	_server.send_to_client(m_nickname, "341 " + m_nickname + ": you invited " + client.get_nickname() + " to join " + channel.get_name() + "\r\n");
	_server.send_to_client(client.m_nickname, "341 " + client.get_nickname() + ": you were invited by " + m_nickname + " to join " + channel.get_name() + "\r\n");
}

void Client::handle_invite(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please register first");

	std::istringstream	iss(buffer);
	std::string nickname;
	std::string channel_name;

	iss >> nickname >> channel_name;
	if (nickname.empty() || channel_name.empty() || are_remain_args(iss))
		// throw std::invalid_argument("invalid input format\nusage: INVITE <nickname> <channel>");
		throw std::invalid_argument("461 " + m_nickname + " INVITE: Not enough parameters");
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + ": " + channel_name + ": No such channel");
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, nickname);
	if (input_client == NULL)
		throw std::invalid_argument("401 " + m_nickname + ": " + nickname + ": No such nick/channel");
	if (is_member(input_channel->get_registered(), m_nickname) == false)
		throw std::invalid_argument("442 " + m_nickname + ": " + channel_name + ": You're not on that channel");
	if (input_channel->get_invite_only() && is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("482 " + m_nickname + ": " + channel_name + ": You're not channel operator");
	if (is_member(input_channel->get_registered(), nickname))
		throw std::invalid_argument("443 " + m_nickname + ": " + nickname + " " + channel_name + ": is already on channel");
	if (is_invited(input_channel, nickname))
		return ;
	invite_client(*input_client, *input_channel);
}

void Client::handle_mode(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please register first");

	const std::map<std::string, Channel*>& channels = _server.get_channels();
	std::istringstream	iss(buffer);
	std::string channel_name;
	std::string type;
	std::string mode;
	std::string mode_param;

	iss >> channel_name >> mode >> mode_param;
	if (channel_name.empty()) // check If there are more params than it should
		throw std::invalid_argument("461 " + m_nickname + " MODE: Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: MODE <channel> ( \"-\" / \"+\" ) ( \"i\" / \"t\" / \"k\" / \"o\" / \"l\" ) [<modeparam>]");
	if (mode[0] != '+' && mode[0]!= '-')
		throw std::invalid_argument("472 " + m_nickname + ": " + mode[0] + ": is unknown mode char to me for " + channel_name);
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + ": " + channel_name + ": No such channel");
	if (is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("482 " + m_nickname + ": " + channel_name + ": You're not channel operator");
	if (mode.empty())
	{
		_server.send_to_client(m_nickname, "324 " + m_nickname + ": " + channel_name + ": modes: " + input_channel->get_modes() + "\r\n");
		return ;
	}
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, mode_param);
	std::string response;
	switch (mode[1])
	{
		case ('i'):
			if (mode[0] == '+')
				input_channel->set_invite_only(true);
			else
				input_channel->set_invite_only(false);
			break ;
		case ('t'):
			if (mode[0] == '+')
				input_channel->set_topic_restriciton(true);
			else
				input_channel->set_topic_restriciton(false);
			break ;
		case ('k'):
			if (mode[0] == '+')
				input_channel->set_key(mode_param);
			else
				input_channel->set_key_needed(false);
			break ;
		case ('o'):
			if (mode_param.empty())
				throw std::invalid_argument("461 " + m_nickname + " MODE: Not enough parameters");
			if (input_client == NULL)
				throw std::invalid_argument("406 " + m_nickname + ": " + mode_param + ": There was no such nickname");
			if (is_member(input_channel->get_registered(), mode_param) == false)
				throw std::invalid_argument("441 " + m_nickname + ": " + mode_param + ": " + channel_name + " :You aren't on that channel");
			if (mode[0] == '+')
			{
				if (is_operator(*input_channel, mode_param))
					throw std::invalid_argument("666 " + m_nickname + ": " + mode_param + ": " + channel_name + ": is already a channel operator");
				input_channel->set_operator(mode_param, give);
			}
			else
				input_channel->set_operator(mode_param, take);
			handle_privmsg(input_channel->get_name() + " : " + mode_param + " was made " + mode[0] + "o by " + m_nickname);
			break ;
		case ('l'):
			if (mode_param.empty())
				throw std::invalid_argument("461 " + m_nickname + " MODE: Not enough parameters");
			if (mode[0] == '+')
			{
				std::stringstream ss(mode_param);
				std::stringstream ss_str_limit(mode_param);
				unsigned short limit;
				ss >> limit;
				std::string str_limit;
				ss_str_limit >> str_limit;
				if (limit <= 0)
					throw std::invalid_argument("696 " + m_nickname + ": " + str_limit + ": is not a valid limit (must be > 0)");
				input_channel->set_user_limit(limit);
			}
			else
				input_channel->set_user_limit(0);
			break ;
		default:
			throw std::invalid_argument("472 " + m_nickname + ": " + mode[1] + ": is unknown mode char to me for " + channel_name);
			break ;
	}
}

void Client::kick_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_nicks) const
{
	std::string							channel_list, nick_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> nick_list;

	if (channel_list.empty() || are_remain_args(iss))
		throw std::invalid_argument("461 " + m_nickname + ": KICK: Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: KICK <channel> *(\",\" <channel>) <user>  *(\",\" <user>)");
	std::istringstream channel_stream(channel_list);
	std::istringstream nick_stream(nick_list);
	std::string channel, nick;
	while (std::getline(channel_stream, channel, ',')) {
		if (!std::getline(nick_stream, nick, ','))
			throw std::invalid_argument("461 " + m_nickname + ": KICK: Not enough parameters");
		channels_to_nicks[channel] = nick;
	}
	if (std::getline(nick_stream, nick, ','))
		throw std::invalid_argument("461 " + m_nickname + ": KICK: Not enough parameters");
}

void Client::kick_user(std::map<std::string, std::string>& channels_to_nick, const std::map<std::string, Channel*>& channels, const std::map<int, Client*>& clients)
{
	for (std::map<std::string, std::string>::iterator it = channels_to_nick.begin(); it != channels_to_nick.end(); ++it)
	{
		Channel* input_channel = find_channel(channels, it->first);
		if (input_channel == NULL)
			throw std::invalid_argument("403 " + m_nickname + " " + it->first + ": No such channel");
		Client* input_client = find_client(clients, it->second);
		if (input_client == NULL)
			throw std::invalid_argument("406 " + m_nickname + ": " + it->second + ": There was no such nickname");
		if (!is_member(input_channel->get_registered(), it->second))
			throw std::invalid_argument(" 441" + m_nickname + ": " + it->second + ": " + it->first + ": They aren't on that channel");
		if (!is_operator(*input_channel, m_nickname))
			throw std::invalid_argument("482 " + m_nickname + ": " + it->first + ": You're not channel operator");
		handle_privmsg(it->second + " you were kicked out of channel " + it->first + "\r\n");
		handle_privmsg(it->first + " " + it->second + " got kicked out of channel " + it->first + "\r\n");
		input_channel->erase_user(it->second);
		if (input_channel->get_registered().size() == 0)
			_server.erase_channel(it->first);
	}
}

void Client::handle_kick(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please register first");

	std::map<std::string, std::string> channels_to_nicks;
	try
	{
		kick_parser(buffer, channels_to_nicks);
	}
	catch(const std::invalid_argument& e)
	{
		throw std::invalid_argument(e.what());
	}
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	const std::map<int, Client*>& clients = _server.get_clients();
	kick_user(channels_to_nicks, channels, clients);
	// channel ceases to exist when the last user leaves it
}

void Client::handle_topic(const std::string& buffer)
{
	std::string							channel_name, topic_name;
	std::istringstream					iss(buffer);
	iss >> channel_name;

	if (!m_is_registered)
		throw std::invalid_argument("451 " + m_nickname + ": Please register first");
	// handle if there are more args than it should
	if (channel_name.empty())
		throw std::invalid_argument("461 " + m_nickname + ": TOPIC: Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: TOPIC <channel> [<topic>]");
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + ": " + channel_name + ": No such channel");
	if (input_channel->get_topic_restriciton() && !is_operator(*input_channel, m_nickname))
		throw std::invalid_argument("482 " + m_nickname + ": " + channel_name + ": You're not channel operator");
	iss >> topic_name;
	if (topic_name.empty())
	{
		const std::string& topic = input_channel->get_topic();
		if (topic.empty())
			throw std::invalid_argument("331 " + m_nickname + ": " + channel_name + ": No topic is set");
		else
			_server.send_to_client(m_nickname, "332 " + m_nickname + ": " + channel_name + " : " + topic + std::string("\r\n"));
	}
	else if (topic_name == "\"\"" || topic_name == "\'\'") // if topic input is empty
		input_channel->set_topic("");	// set topic to empty (delete topic)
	else
		input_channel->set_topic(topic_name); // set new topic
}


void Client::handle_who(const std::string& buffer)
{
	std::string							channel_name, o_parameter;
	std::istringstream					iss(buffer);
	iss >> channel_name >> o_parameter;

	if (channel_name.empty())
	{
		const std::map<int, Client *> clients = _server.get_clients();
		for (std::map<int, Client*>::const_iterator it_clients = clients.begin(); it_clients != clients.end(); ++it_clients)
			_server.send_to_client(m_nickname, "352" + m_nickname + " * " + it_clients->second->get_username() + " * ft_irc " + it_clients->second->get_nickname() + " H :0 " +  "REAL\r\n");
	}
	else if (o_parameter.empty())
	{
		Channel *channel = find_channel(_server.get_channels(), channel_name);
		if (channel == NULL)
			throw std::invalid_argument("403 " + m_nickname + " " + channel_name + " :No such channel");
		const std::set<Client*>& registered = channel->get_registered();
		// maybe erase the first character of channel_name
		for (std::set<Client*>::const_iterator it_registered = registered.begin(); it_registered != registered.end(); ++it_registered)
			_server.send_to_client(m_nickname, "352 " + m_nickname + " " + channel_name + " " + (*it_registered)->get_username() + " * ft_irc " + (*it_registered)->get_nickname() + " H :0 " +  "REAL\r\n");
	}
	else if (o_parameter == "o")
	{
		Channel *channel = find_channel(_server.get_channels(), channel_name);
		if (channel == NULL)
			throw std::invalid_argument("403 " + m_nickname + " " + channel_name + " :No such channel");
		const std::set<std::string> operators = channel->get_operators();
		// maybe erase the first character of channel_name
		for (std::set<std::string>::const_iterator it_operators = operators.begin(); it_operators != operators.end(); ++it_operators)
		{
			Client *client = find_client(_server.get_clients(), *it_operators);
			_server.send_to_client(m_nickname, "352 " + m_nickname + " " + channel_name + " " + client->get_username() + " * ft_irc " + client->get_nickname() + " G :0 " +  "REAL\r\n");
		}
	}
	_server.send_to_client(m_nickname, "315 * :" + m_nickname + "\r\n");
}
