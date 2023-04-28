
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
	// m_commands["INVITE"] = &Client::handle_invite;
	// m_commands["MODE"] = &Client::handle_mode;

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

bool Client::is_invited(const Channel* channel)
{
	std::set<Client*> invited = channel->get_invited();

	for(std::set<Client*>::iterator it = invited.begin(); it != invited.end(); ++it)
	{
		if ((*it)->m_nickname == m_nickname)
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
			if (it_channel->second->get_invite_only() && !is_invited(it_channel->second))
				throw std::invalid_argument("you are not invited to " + it_channel->first + "\n");
			if (it_channel->second->get_key_needed() && !is_valid_key(it_channel->second, it->second))
				throw std::invalid_argument("wrong key for " + it_channel->first + "\n");
			it_channel->second->set_registered(*this);
			std::cout << "you are registered to " << it_channel->first << "\n";
		}
		else
		{
			Channel* new_channel = new Channel(it->first, it->second);
			new_channel->set_registered(*this);
			_server.add_new_channel(new_channel);
		}
	}
}

void Client::join_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_keys)
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	std::string other;
	iss >> other;
	if (!key_list.empty() && !other.empty())
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
		if (it->first[0] != '#')
			throw std::invalid_argument("channel names must be prefixed by a '#'");
		if (it->first.find(',') != std::string::npos || it->first.find((char)7) != std::string::npos)
		{
			std::cout << it->first << std::endl;
			throw std::invalid_argument("channel name shall not contain control G or a comma");
		}
		if (it->first.length() > 50)
			throw std::invalid_argument("channel names are at most fifty (50) charachters long");
		// maybe try to handle the "channel name shall not contain any spaces"
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

void handle_invite(const std::string& buffer)
{
	std::istringstream	iss(buffer);
	std::string nickname;
	std::string channel;
	std::string other;

	iss >> nickname >> channel >> other;
	if (nickname.empty() || channel.empty() || !other.empty())
		throw std::invalid_argument("invalid input format\nusage: INVITE <nickname> <channel>");
}
