
// client.cpp
#include "client.hpp"
#include "Channel.hpp"
#include <sstream>
#include <iostream>
#include <exception>

Client::Client(int fd, Server& server) : m_fd(fd), m_is_registered(false), _server(server), m_nickname(""), m_username(""), m_authenticated(false), buffer("")
{
	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
	m_commands["JOIN"] = &Client::handle_join;
	// m_commands["KICK"] = &Client::handle_kick;
	// m_commands["TOPIC"] = &Client::handle_topic;
	m_commands["INVITE"] = &Client::handle_invite;
	m_commands["MODE"] = &Client::handle_mode;
	m_commands["PRIVMSG"] = &Client::handle_privmsg;
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

bool Client::is_registered(const Channel* channel)
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
			if (is_registered(it_channel->second))
				throw std::invalid_argument("you are already registered to channel " + it_channel->first + "\n");
			if (it_channel->second->get_invite_only() && !is_invited(it_channel->second, m_nickname))
				throw std::invalid_argument("invite-only channel: you are not invited to " + it_channel->first + "\n");
			if (it_channel->second->get_key_needed() && !is_valid_key(it_channel->second, it->second))
				throw std::invalid_argument("wrong key for " + it_channel->first + "\n");
			it_channel->second->set_registered(*this);
			std::cout << "you are registered to " << it_channel->first << "\n";
		}
		else
		{
			Channel* new_channel = new Channel(it->first, it->second, m_nickname);
			new_channel->set_registered(*this);
			_server.add_new_channel(new_channel);
			std::cout << "you created channel " << new_channel->get_name() << "\n";
		}
	}
}

bool are_remain_args(std::istringstream& iss)
{
	std::string other;

	if ((iss >> other))
		return (true);
	return (false);
}

void Client::join_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_keys)
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	if (are_remain_args(iss))
		throw std::invalid_argument("invalid input format\nusage: JOIN <channel> *(\",\" <channel>) [<key> *(\",\" <key>)]");
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
		if (it->first[0] != '#' || it->first.length() == 1)
			throw std::invalid_argument("channel names must be prefixed by a '#' and contain at leats one character");
		if (it->first.find(',') != std::string::npos || it->first.find((char)7) != std::string::npos)
			throw std::invalid_argument("channel name shall not contain control G or a comma");
		if (it->first.length() > 50)
			throw std::invalid_argument("channel names are at most fifty (50) charachters long");
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
			if (!args.empty() && args[0] == ' ') {
				args.erase(0, 1); // Remove the leading space

/*			try
			{
				(this->*(it->second))(command.substr(command.find(' ') + 1));
				return ;
			}
			catch(const std::exception& e)
			{
				std::cerr << "Error\n" << e.what() << '\n';
				return ; */

			}
			(this->*(it->second))(args);
			return ;
		}
	}
	std::cerr << "Error\n" << "invalid input\n";
}

void Client::handle_pass(const std::string &args) {
	if (m_authenticated)
		return ;

	std::string arg;
	std::istringstream iss(args);
	iss >> arg;

	if (arg != _server.get_password()) {
		// Send an error response
		std::cout << "Wrong password" << std::endl;
		return;
	}

	m_authenticated = true;
	// Send a welcome response
	std::cout << "Client " << m_fd << " authenticated" << std::endl;

}

void Client::handle_user(const std::string &args) {
	if (!m_authenticated){
		std::cout << "Error: USER command requires an authenticated client." << std::endl;
		return;
	}

	std::istringstream iss(args);
	std::string arg;

	if (!(iss >> arg)) {
		std::cout << "Error: USER command requires a username" << std::endl;
		return;
	}

	if (arg[0] == '#') {
		std::cout << "Error: Nickname cannot start with '#'" << std::endl;
		return;
	}

	m_username = arg;
	std::cout << "Client " << m_fd << " set username to: " << m_username << std::endl;
}

void Client::handle_nick(const std::string &args) {
	if (!m_authenticated || m_username.empty()){
		std::cout << "Error: NICK command requires an authenticated client with username." << std::endl;
		return ;
	}

	std::string arg;
	std::istringstream iss(args);

	if (!(iss >> arg)) {
		std::cout << "Error: NICK command requires a nickname." << std::endl;
		return;
	}

	if (arg[0] == '#') {
		std::cout << "Error: Nickname cannot start with '#'" << std::endl;
		return;
	}

	m_nickname = arg;
	m_is_registered = true;
	std::cout << "Client " << m_fd << " set nickname to: " << m_nickname << std::endl;
}

void Client::handle_privmsg(const std::string& args) {
	if (!m_is_registered){
		std::cout << "Error: PRIVMSG command requires a fully registered client." << std::endl;
		return ;
	}

	if (args.empty()) {
		std::cout << "Error: No recipient for PRIVMSG" << std::endl;
		return;
	}

	std::string target, message;
	std::istringstream iss(args);
	iss >> target;

	if (!std::getline(iss, message)) {
		std::cout << "Error: No text to send" << std::endl;
		return;
	}

	const std::map<std::string, Channel*>& channels = _server.get_channels();

	// Check if the target is an existing channel
	std::map<std::string, Channel*>::const_iterator it = channels.find(target);
	if (it != channels.end()) {
		Channel* channel = it->second;
		const std::set<Client*>& registered_clients = channel->get_registered();

		if (is_registered(channel)) {
			for (std::set<Client*>::iterator it = registered_clients.begin(); it != registered_clients.end(); ++it) {
				Client* client = *it;
				if (client != this) {
					std::string full_message = m_nickname + " to " + target + ": " + message + "\r\n";
					send(client->m_fd, full_message.c_str(), full_message.size(), 0);
				}
			}
		} else
			std::cout << "Error: Cannot send to channel " << target << std::endl;
	}
	else {
		// If the target is not an existing channel, it's a private message to a user or a non-existing channel
		if (!_server.send_to_client(target, m_nickname + ": " + message + "\r\n")) {
			if (target[0] == '#')
				std::cout << "Error: No such channel " << target << std::endl;
			else
				std::cout << "Error: No such nickname " << target << std::endl;
		}
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
	handle_privmsg(m_nickname + " you invited " + client.get_nickname() + " to channel " + channel.get_name() + "\n");
	handle_privmsg(client.m_nickname + " you were invited by " + client.get_nickname() + " to join channel " + channel.get_name() + "\n");
}

void Client::handle_invite(const std::string& buffer)
{
	std::istringstream	iss(buffer);
	std::string nickname;
	std::string channel_name;

	iss >> nickname >> channel_name;
	if (nickname.empty() || channel_name.empty() || are_remain_args(iss))
		throw std::invalid_argument("invalid input format\nusage: INVITE <nickname> <channel>");
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("channel does not exist");
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, nickname);
	if (input_client == NULL)
		throw std::invalid_argument("user does not exist");
	if (input_channel->get_invite_only() && is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("this channel is invite-only: non-channel-operators are not allowed to send invitations");
	if (is_member(input_channel->get_registered(), m_nickname) == false)
		throw std::invalid_argument("non-members are not allowed to send invitations");
	if (is_member(input_channel->get_registered(), nickname))
		throw std::invalid_argument("the user you are trying to invite is already a member of " + input_channel->get_name());
	if (is_invited(input_channel, m_nickname)) // FIX THIS
		throw std::invalid_argument("the user you are trying to invite is already invited to channel " + input_channel->get_name());
	invite_client(*input_client, *input_channel);
}



// <channel> *( ( "-" / "+" ) *<modes> *<modeparams> )
void Client::handle_mode(const std::string& buffer)
{
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	std::istringstream	iss(buffer);
	std::string channel_name;
	std::string type;
	std::string mode;
	std::string mode_param;
	// const std::string modes[] = {"i", "t", "k", "o", "l"};

	iss >> channel_name >> type >> mode >> mode_param;
	if (channel_name.empty() || type.empty() || (type != "-" && type != "+") || mode.empty() || mode.length() != 1) // check If there are more params than it should
		throw std::invalid_argument("invalid input format\nusage: MODE <channel> ( \"-\" / \"+\" ) ( \"i\" / \"t\" / \"k\" / \"o\" / \"l\" ) [<modeparam>]");
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument("channel does not exist");
	if (is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument("non-channel-operators are not allowed to use MODE");
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, mode_param);
	switch (mode[0])
	{
		case ('i'):
			if (type[0] == '+')
				input_channel->set_invite_only(true);
			else
				input_channel->set_invite_only(false);
			break ;
		case ('t'):
			if (type[0] == '+')
				input_channel->set_topic_restriciton(true);
			else
				input_channel->set_topic_restriciton(false);
			break ;
		case ('k'):
			if (type[0] == '+')
				input_channel->set_key(mode_param);
			else
				input_channel->set_key_needed(false);
			break ;
		case ('o'):
			if (input_client == NULL)
				throw std::invalid_argument("user does not exist");
			if (is_member(input_channel->get_registered(), mode_param) == false)
				throw std::invalid_argument("user is not a member of channel " + input_channel->get_name());
			std::cout << "JO\n";
			if (type[0] == '+')
			{
				if (is_operator(*input_channel, mode_param))
					throw std::invalid_argument("user is already an operator of channel " + input_channel->get_name());
				input_channel->set_operator(mode_param, give);
			}
			else
				input_channel->set_operator(mode_param, take);
			break ;
		default:
			throw std::invalid_argument("invalid mode\n");
			break ;
	}
}