
// client.cpp
#include "client.hpp"
#include "Channel.hpp"
#include <sstream>
#include <iostream>
#include <exception>

Client::Client(int fd, Server& server) : m_fd(fd), m_is_registered(false), _server(server)
{
	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
	m_commands["JOIN"] = &Client::handle_join;
	// m_commands["KICK"] = &Client::handle_kick;
	// m_commands["TOPIC"] = &Client::handle_topic;
	// m_commands["INVITE"] = &Client::handle_invite;
	// m_commands["MODE"] = &Client::handle_mode;
}

void Client::check_channel_status(std::map<std::string, std::string> &channels_to_keys)
{
	std::map<std::string, Channel*> channels = _server.get_channels();
	for (std::map<std::string, std::string>::iterator it = channels_to_keys.begin(); it != channels_to_keys.end(); ++it)
	{
		for (std::map<std::string, Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
		{
			
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
		throw std::invalid_argument("invalid input format\nusage: <channel> *(\",\" <channel>) [<key> *(\",\" <key>)]");
	std::istringstream channel_stream(channel_list);
	std::istringstream key_stream(key_list);
	std::string channel, key;
	while (std::getline(channel_stream, channel, ',')) {
		if (!std::getline(key_stream, key, ','))
			key = "";
		channels_to_keys[channel] = key;
	}
	std::cout << channels_to_keys.begin()++->first.find(',');
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
	check_channel_status(channels_to_keys);
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
		std::cerr << "Error\n" << e.what() << '\n';
		return ;
	}
}

void Client::parse_command(const std::string &command) {
	std::istringstream iss(command);
	std::string cmd;
	iss >> cmd;

	(void) m_fd;

	for (std::map<std::string, CommandHandler>::iterator it = m_commands.begin(); it != m_commands.end(); ++it)
	{
		if (cmd == it->first)
		{
			(this->*(it->second))(command.substr(command.find(' ') + 1)); // here I changed because we werent getting the whole buffer
			return ;
		}
	}
	std::cout << "Suck a D" << std::endl;
}

// MODE (t, i, m, b)

void Client::handle_pass(const std::string &arg) {
	if (m_is_registered)
		return ;

	if (arg != _server.get_password()) {
		// Send an error response
		std::cout << "Wrong password" << std::endl;
		return;
	}

	m_is_registered = true;
	// Send a welcome response
	std::cout << "Client " << m_fd << " registered" << std::endl;

}

void Client::handle_nick(const std::string &arg) {
	if (arg.empty()) {
		std::cout << "Error: NICK command requires an argument." << std::endl;
		return;
	}

	m_nickname = arg;
	std::cout << "Client " << m_fd << " set nickname to: " << m_nickname << std::endl;
}

void Client::handle_user(const std::string &arg) {
	std::istringstream iss(arg);
	std::string username, realname;

	if (!(iss >> username)) {
		std::cout << "Error: USER command requires a username" << std::endl;
		return;
	}
	m_username = username;

	std::cout << "Client " << m_fd << " set username to: " << m_username << std::endl;
}
