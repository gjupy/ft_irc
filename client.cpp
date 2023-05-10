
// client.cpp
#include "client.hpp"
#include "Channel.hpp"
#include <sstream>
#include <iostream>
#include <exception>

Client::Client(int fd, Server& server) : m_fd(fd), m_is_registered(false), _server(server), m_nickname("*"), m_username(""), m_authenticated(false), buffer("")
{
	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
	m_commands["JOIN"] = &Client::handle_join;
	m_commands["KICK"] = &Client::handle_kick;
	m_commands["TOPIC"] = &Client::handle_topic;
	m_commands["INVITE"] = &Client::handle_invite;
	m_commands["MODE"] = &Client::handle_mode;
	m_commands["PRIVMSG"] = &Client::handle_privmsg;
	m_commands["PING"] = &Client::handle_ping;
	m_commands["CAP"] = &Client::handle_cap;
}

const std::string& Client::get_nickname() const {
	return m_nickname;
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
				throw std::invalid_argument("471 " + m_nickname + " " + it_channel->first + " :Cannot join channel (+l)");
			if (it_channel->second->get_invite_only() && !is_invited(it_channel->second, m_nickname))
				throw std::invalid_argument("473 " + m_nickname + " " + it_channel->first + " :Cannot join channel (+i)");
			if (it_channel->second->get_key_needed() && !is_valid_key(it_channel->second, it->second))
				throw std::invalid_argument("475 " + m_nickname + " " + it_channel->first + " :Cannot join channel (+k)");
			it_channel->second->set_registered(*this);
			_server.send_to_client(m_nickname, ":" + m_nickname + " JOIN :" + it_channel->second->get_name() + "\r\n");
		}
		else
		{
			Channel* new_channel = new Channel(it->first, it->second, m_nickname);
			new_channel->set_registered(*this);
			_server.add_new_channel(new_channel);
			std::cout << ":" + m_nickname + " JOIN :" + new_channel->get_name() + "\r\n";
			_server.send_to_client(m_nickname, ":" + m_nickname + " JOIN :" + new_channel->get_name() + "\r\n");
		}
	}
}

bool are_remain_args(std::istringstream& iss)
{
	std::string other;

	iss >> other;
	if (!other.empty())
		return (true);
	return (false);
}

void Client::join_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_keys)
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	if (channel_list.empty() || are_remain_args(iss))
		// throw std::invalid_argument("invalid input format\nusage: JOIN <channel> *(\",\" <channel>) [<key> *(\",\" <key>)]");
		throw std::invalid_argument("461 " + m_nickname + " JOIN :Not enough parameters");
	std::istringstream channel_stream(channel_list);
	std::istringstream key_stream(key_list);
	std::string channel, key;
	while (std::getline(channel_stream, channel, ',')) {
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
			throw std::invalid_argument("403 " + m_nickname + " " + it->first + " :No such channel");
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

void Client::handle_ping(const std::string& args)
{
	if (args.size() < 1) {
		// Send an error message to the client.
		_server.send_to_client(m_nickname, "461 " + m_nickname + " PING" + "\r\n");
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

/*
c1 *(,cn) *(key) *(, key)
keys are assigned to the channels in the order they are written
*/
void Client::handle_join(const std::string& buffer)
{
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
				// (this->*(it->second))(command.substr(command.find(' ') + 1));
				return ;
			}
			catch(const std::exception& e)
			{
				_server.send_to_client(m_nickname, e.what() + std::string("\r\n"));
				return ;
			}
			// (this->*(it->second))(args);
			// return ;
		}
	}
	std::cerr << "Error\n" << "invalid input\n";
}

void Client::handle_pass(const std::string &args) {
	if (m_authenticated)
		throw std::invalid_argument("462 " + m_nickname + " :You are already registered");

	std::string arg;
	std::istringstream iss(args);
	iss >> arg;

	if (arg != _server.get_password())
		throw std::invalid_argument("464 " + m_nickname + " :Password incorrect");

	m_authenticated = true;
	std::cout << "Client " << m_fd << " authenticated" << std::endl;
}


void Client::handle_user(const std::string &args) {
	if (!m_authenticated)
		throw std::invalid_argument("461 " + m_nickname + " :Please provide a server password using PASS command");

	if (!m_username.empty())
		throw std::invalid_argument("462 " + m_nickname + " :You are already registered");

	std::istringstream iss(args);
	std::string arg;

	if (!(iss >> arg || arg[0] == '#'))
		throw std::invalid_argument("461 " + m_nickname + " USER :Not correct usage of USER command");

	m_username = arg;
	std::cout << "Client " << m_fd << " set username to: " << m_username << std::endl;
}

void Client::handle_nick(const std::string &args) {
	if (!m_authenticated || m_username.empty())
		throw std::invalid_argument("461 " + m_nickname + " :You need to register using PASS and USER commands");

	std::string arg;
	std::istringstream iss(args);

	if (!(iss >> arg || arg[0] == '#'))
		throw std::invalid_argument("461 " + m_nickname + " NICK :Not correct usage of NICK command");

	m_nickname = arg;
	m_is_registered = true;
	std::cout << "Client " << m_fd << " set nickname to: " << m_nickname << std::endl;
	_server.send_to_client(m_nickname, "001 " + m_nickname + " :Welcome to the Internet Relay Network, " + m_nickname + "!" + "\r\n"); // 001: RPL_WELCOME
	// _server.send_to_client(m_nickname, "002 " + m_nickname + " :Your host is " + std::string(HOST) + ", running version 1.0" + "\r\n"); // 002: RPL_YOURHOST
}

void Client::handle_privmsg(const std::string& args) {
	if (!m_is_registered)
		throw std::invalid_argument("461 " + m_nickname + " :Please set both USER and NICK before using other commands");
	// ERR_NEEDMOREPARAMS
	if (args.empty())
		throw std::invalid_argument("411 " + m_nickname + " :No recipient given (PRIVMSG)");

	std::string target, message;
	std::istringstream iss(args);
	iss >> target;

	if (!std::getline(iss, message))
		throw std::invalid_argument("412 " + m_nickname + " :No text to send");

	const std::map<std::string, Channel*>& channels = _server.get_channels();

	// Check if the target is an existing channel
	std::map<std::string, Channel*>::const_iterator it = channels.find(target);
	if (it != channels.end()) {
		Channel* channel = it->second;
		const std::set<Client*>& registered_clients = channel->get_registered();

		if (is_registered_channel(channel)) {
			for (std::set<Client*>::iterator it = registered_clients.begin(); it != registered_clients.end(); ++it) {
				Client* client = *it;
				if (client != this) {
					std::string response = ":" + m_nickname + " PRIVMSG " + target + " :" + message + "\r\n";
					_server.send_to_client(client->get_nickname(), response);
				}
			}
		} else
			throw std::invalid_argument("404 " + m_nickname + " " + target + " :Cannot send to channel");
	}
	else {
		// If the target is not an existing channel, it's a private message to a user or a non-existing channel
		std::string response = ":" + m_nickname + " PRIVMSG " + target + " :" + message + "\r\n";
		if (!_server.send_to_client(target, response))
			throw std::invalid_argument("401 " + m_nickname + " " + target + " :No such nick/channel");
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
	_server.send_to_client(m_nickname, "341 " + client.get_nickname() + " " + channel.get_name() + " " + client.get_nickname() + "\r\n");
	_server.send_to_client(client.m_nickname, "341 " + client.get_nickname() + " " + channel.get_name() + " " + client.get_nickname() + "\r\n");
}

void Client::handle_invite(const std::string& buffer)
{
	std::istringstream	iss(buffer);
	std::string nickname;
	std::string channel_name;

	iss >> nickname >> channel_name;
	if (nickname.empty() || channel_name.empty() || are_remain_args(iss))
		// throw std::invalid_argument("invalid input format\nusage: INVITE <nickname> <channel>");
		throw std::invalid_argument("461 " + m_nickname + " INVITE :Not enough parameters");
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + " " + channel_name + " :No such channel");
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, nickname);
	if (input_client == NULL)
		throw std::invalid_argument("401 " + m_nickname + " " + nickname + " :No such nick/channel");
	if (is_member(input_channel->get_registered(), m_nickname) == false)
		throw std::invalid_argument("442 " + m_nickname + " " + channel_name + " :You're not on that channel");
	if (input_channel->get_invite_only() && is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("482 " + m_nickname + " " + channel_name + " :You're not channel operator");
	if (is_member(input_channel->get_registered(), nickname))
		throw std::invalid_argument("443 " + m_nickname + " " + nickname + " " + channel_name + " :is already on channel");
	if (is_invited(input_channel, nickname))
		return ;
	invite_client(*input_client, *input_channel);
}

void Client::handle_mode(const std::string& buffer)
{
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	std::istringstream	iss(buffer);
	std::string channel_name;
	std::string type;
	std::string mode;
	std::string mode_param;

	iss >> channel_name >> mode >> mode_param;
	if (channel_name.empty()) // check If there are more params than it should
		throw std::invalid_argument("461 " + m_nickname + " MODE :Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: MODE <channel> ( \"-\" / \"+\" ) ( \"i\" / \"t\" / \"k\" / \"o\" / \"l\" ) [<modeparam>]");
	if (mode[0] != '+' && mode[0]!= '-')
		throw std::invalid_argument("472 " + m_nickname + " " + mode[0] + " :is unknown mode char to me for " + channel_name);
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + " " + channel_name + " :No such channel");
	if (is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("482 " + m_nickname + " " + channel_name + " :You're not channel operator");
	if (mode.empty())
	{
		// OR HOSTNAME
		_server.send_to_client(m_nickname, ":" + std::string(SERVERNAME) + " 324 " + m_nickname + " " + channel_name + " " + input_channel->get_modes() + "\r\n");
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
				throw std::invalid_argument("461 " + m_nickname + " MODE :Not enough parameters");
			if (input_client == NULL)
				throw std::invalid_argument("406 " + m_nickname + " " + mode_param + " :There was no such nickname");
			if (is_member(input_channel->get_registered(), mode_param) == false)
				throw std::invalid_argument(" 441" + m_nickname + " " + mode_param + " " + channel_name + " :They aren't on that channel");
			if (mode[0] == '+')
			{
				if (is_operator(*input_channel, mode_param))
					return ; // USER IS ALREADY OPERATOR
				input_channel->set_operator(mode_param, give);
			}
			else
				input_channel->set_operator(mode_param, take);
			response = ":" + m_nickname + "!~" + m_username + "@" + std::string(HOST) + " MODE " + channel_name + " " + mode + " " + mode_param;
			// sendResponseToChannel(client, channelName, response);
				// send message to everyone on channel other than the sender
			break ;
		case ('l'):
			if (mode_param.empty())
				throw std::invalid_argument("461 " + m_nickname + " MODE :Not enough parameters");
			if (mode[0] == '+')
			{
				std::stringstream ss(mode_param);
				unsigned short limit;
				ss >> limit;
				if (limit <= 0)
					return ;
				input_channel->set_user_limit(limit);
			}
			else
				input_channel->set_user_limit(0);
			break ;
		default:
			throw std::invalid_argument("472 " + m_nickname + " " + mode[1] + " :is unknown mode char to me for " + channel_name);
			break ;
	}
}

void Client::kick_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_nicks) const
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	if (channel_list.empty() || are_remain_args(iss))
		throw std::invalid_argument("461 " + m_nickname + " KICK :Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: KICK <channel> *(\",\" <channel>) <user>  *(\",\" <user>)");
	std::istringstream channel_stream(channel_list);
	std::istringstream nick_stream(key_list);
	std::string channel, nick;
	while (std::getline(channel_stream, channel, ',')) {
		if (!std::getline(nick_stream, nick, ','))
			throw std::invalid_argument("461 " + m_nickname + " KICK :Not enough parameters");
		channels_to_nicks[channel] = nick;
	}
	if (std::getline(nick_stream, nick, ','))
		throw std::invalid_argument("461 " + m_nickname + " KICK :Not enough parameters");
}

void Client::kick_user(std::map<std::string, std::string>& channels_to_nick, const std::map<std::string, Channel*>& channels, const std::map<int, Client*>& clients)
{
	for (std::map<std::string, std::string>::iterator it = channels_to_nick.begin(); it != channels_to_nick.end(); ++it)
	{
		Channel* input_channel = find_channel(channels, it->first);
		if (input_channel == NULL)
			throw std::invalid_argument("403 " + m_nickname + " " + it->first + " :No such channel");
		Client* input_client = find_client(clients, it->second);
		if (input_client == NULL)
			throw std::invalid_argument("406 " + m_nickname + " " + it->second + " :There was no such nickname");
		if (!is_member(input_channel->get_registered(), it->second))
			throw std::invalid_argument(" 441" + m_nickname + " " + it->second + " " + it->first + " :They aren't on that channel");
		if (!is_operator(*input_channel, m_nickname))
			throw std::invalid_argument("482 " + m_nickname + " " + it->first + " :You're not channel operator");
		input_channel->erase_user(it->second);
		_server.send_to_client(it->second, ":" + m_nickname + " KICK " + it->first + " " + it->second + " :" + "\r\n");
	}
}

void Client::handle_kick(const std::string& buffer)
{
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

	// handle if there are more args than it should
	if (channel_name.empty())
		throw std::invalid_argument("461 " + m_nickname + " TOPIC :Not enough parameters");
		// throw std::invalid_argument("invalid input format\nusage: TOPIC <channel> [<topic>]");
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("403 " + m_nickname + " " + channel_name + " :No such channel");
	if (input_channel->get_topic_restriciton() && !is_operator(*input_channel, m_nickname))
		throw std::invalid_argument("482 " + m_nickname + " " + channel_name + " :You're not channel operator");
	iss >> topic_name;
	if (topic_name.empty())
	{
		const std::string& topic = input_channel->get_topic();
		if (topic.empty())
			throw std::invalid_argument("331 " + m_nickname + " " + channel_name + " :No topic is set");
		else
			_server.send_to_client(m_nickname, "332 " + m_nickname + " " + channel_name + " :" + topic + std::string("\r\n"));
	}
	else if (topic_name == "\"\"" || topic_name == "\'\'") // if topic input is empty
		input_channel->set_topic("");	// set topic to empty (delete topic)
	else
		input_channel->set_topic(topic_name); // set new topic
}
