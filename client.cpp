/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/05/12 20:15:51 by gjupy             #+#    #+#             */
/*   Updated: 2023/05/12 22:48:55 by gjupy            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
}

Client::~Client()
{
	unregistered_count--;
	std::map<std::string, Channel*> channels = _server.get_channels();
	for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
	{
		if (is_registered_channel(it->second))
			handle_exit(it->first);
	}
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
	if (!m_is_registered)
		throw std::invalid_argument(RED "451 You have not registered to the server\r\n" RESET);
	std::string							channel_name;
	std::istringstream					iss(buffer);

	iss >> channel_name;
	if (channel_name.empty())
		throw std::invalid_argument(RED "461 EXIT: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 EXIT: Too many parameters" RESET);
	Channel *channel = find_channel(_server.get_channels(), channel_name);
	if (channel == NULL)
		throw std::invalid_argument(RED "403 " + channel_name + ": No such channel" RESET);
	if (!is_member(channel->get_registered(), m_nickname))
		throw std::invalid_argument(RED "441 " + channel_name + ": Your aren't on that channel" RESET);
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
				throw std::invalid_argument(RED "888 " + it_channel->first + ": You're already on that channel" RESET);
			if (it_channel->second->get_user_limit() && it_channel->second->get_registered().size() >= it_channel->second->get_user_limit())
				throw std::invalid_argument(RED "471 " + it_channel->first + ": Cannot join channel (+l)" RESET);
			if (it_channel->second->get_invite_only() && !is_invited(it_channel->second, m_nickname))
				throw std::invalid_argument(RED "473 " + it_channel->first + ": Cannot join channel (+i)" RESET);
			if (it_channel->second->get_key_needed() && !is_valid_key(it_channel->second, it->second))
				throw std::invalid_argument(RED "475 " + it_channel->first + ": Cannot join channel (+k)" RESET);
			it_channel->second->set_registered(*this);
			_server.send_to_client(m_nickname, BLUE "you joined channel " + it->first + "\r\n" RESET);
			handle_privmsg(it->first + " " + m_nickname + " joined channel " + it->first + "\r\n");
		}
		else
		{
			Channel* new_channel = new Channel(it->first, it->second, m_nickname);
			new_channel->set_registered(*this);
			_server.add_new_channel(new_channel);
			_server.send_to_client(m_nickname, BLUE "you created channel " + it->first + "\r\n" RESET);
		}
	}
}

void Client::join_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_keys)
{
	std::string							channel_list, key_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> key_list;

	if (channel_list.empty())
		throw std::invalid_argument(RED "461 JOIN: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 JOIN: Too many parameters" RESET);
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
			throw std::invalid_argument(RED "410 " + it->first + ": Erroneous channel name" RESET);
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
		throw std::invalid_argument(RED "451 You have not registered to the server" RESET);

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
	_server.send_to_client(m_nickname, RED "421 " + cmd + ": Unknown command\r\n" RESET);
}

void Client::handle_pass(const std::string &args)
{
	if (m_authenticated)
		throw std::invalid_argument(RED "462 You're already registered" RESET);
	if (args.empty())
		throw std::invalid_argument(RED "461 PASS: Not enough parameters" RESET);
	std::string arg;
	std::istringstream iss(args);
	iss >> arg;

	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 PASS: Too many parameters" RESET);
	if (arg != _server.get_password())
		throw std::invalid_argument(RED "464 Password incorrect" RESET);

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

void Client::handle_user(const std::string &args)
{
	if (!m_authenticated)
		throw std::invalid_argument(RED "451 Please provide a server password using PASS command" RESET);
	if (!m_username.empty())
		throw std::invalid_argument(RED "462 Unauthorized command (already registered)" RESET);
	if (args.empty())
		throw std::invalid_argument(RED "461 USER: Not enough parameters" RESET);
	if (username_exists(args))
		throw std::invalid_argument(RED "433 " + args + ": This username is already in use" RESET);

	std::istringstream iss(args);
	std::string arg;

	if (args.empty() || !(iss >> arg))
		throw std::invalid_argument(RED "461 USER: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 USER: Too many parameters" RESET);
	if (arg[0] == '#' || arg == "*")
		throw std::invalid_argument(RED "438 " + arg + ": Erroneous username" RESET);
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
		throw std::invalid_argument(RED "451 You need to register using PASS and USER commands" RESET);
	if (!m_nickname.empty() && m_nickname.substr(0, 13) != "unregistered_")
		throw std::invalid_argument(RED "462 Unauthorized command (already registered)" RESET);

	std::string arg;
	std::istringstream iss(args);
	if (!(iss >> arg))
		throw std::invalid_argument(RED "461 NICK: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 NICK: Too many parameters" RESET);
	if (nickname_exists(args) || args == m_username)
		throw std::invalid_argument(RED "433 " + args + ": Nickname is already in use" RESET);
	if (arg[0] == '#')
		throw std::invalid_argument(RED "432 " + arg + ": Erroneous nickname" RESET);
	m_nickname = arg;
	m_is_registered = true;
	std::cout << "Client " << m_fd << " set nickname to: " << m_nickname << std::endl;
	_server.send_to_client(m_nickname, GREEN "001 " + m_nickname + ": Welcome to the Internet Relay Network, " + m_nickname + "!\r\n" RESET); // 001: RPL_WELCOME
	_server.send_to_client(m_nickname, GREEN "002 " + m_nickname + ": Your host is " + std::string(HOST) + ", running version IRC_TRASH\r\n" RESET); // 002: RPL_YOURHOST
}

void Client::handle_privmsg(const std::string& args)
{
	if (!m_is_registered)
		throw std::invalid_argument(RED "451 You have not registered to the server" RESET);

	std::string target, message;
	std::istringstream iss(args);
	if (!std::getline(iss, target, ' '))
		throw std::invalid_argument(RED "461 PRIVMSG: Not enough parameters: target missing" RESET);
	if (!std::getline(iss, message))
		throw std::invalid_argument(RED "461 PRIVMSG: Not enough parameters: message missing" RESET);

	Channel *channel = find_channel(_server.get_channels(), target);
	if (channel)
	{
		const std::set<Client*>& registered_clients = channel->get_registered();
		if (is_registered_channel(channel))
		{
			for (std::set<Client*>::iterator it = registered_clients.begin(); it != registered_clients.end(); ++it)
			{
				Client* client = *it;
				if (client->get_nickname() != m_nickname)
				{
					std::string response = m_nickname + ": " + target + ": " + message + "\r\n";
					_server.send_to_client(client->get_nickname(), BLUE "" + response + "" RESET);
				}
			}
		}
		else
			throw std::invalid_argument(RED "404 " + target + ": Cannot send to channel" RESET);
	}
	else
	{
		std::string response = m_nickname + ": " + message + "\r\n";
		if (!_server.send_to_client(target, PURPLE"" + response + "" RESET))
			throw std::invalid_argument(RED "401 " + target + ": No such nick/channel" RESET);
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
	_server.send_to_client(m_nickname, BLUE "341 you invited " + client.get_nickname() + " to join " + channel.get_name() + "\r\n" RESET);
	_server.send_to_client(client.m_nickname, BLUE "341 you were invited by " + m_nickname + " to join " + channel.get_name() + "\r\n" RESET);
}

void Client::handle_invite(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument(RED "451 You have not registered to the server\r\n" RESET);

	std::istringstream	iss(buffer);
	std::string nickname;
	std::string channel_name;

	iss >> nickname >> channel_name;
	if (nickname.empty() || channel_name.empty())
		throw std::invalid_argument(RED "461 INVITE: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 INVITE: Too many parameters" RESET);
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument(RED "403 " + channel_name + ": No such channel" RESET);
	const std::map<int, Client*>& clients = _server.get_clients();
	Client* input_client = find_client(clients, nickname);
	if (input_client == NULL)
		throw std::invalid_argument(RED "401 " + nickname + ": No such nick/channel" RESET);
	if (is_member(input_channel->get_registered(), m_nickname) == false)
		throw std::invalid_argument(RED "442 " + channel_name + ": You're not on that channel" RESET);
	if (input_channel->get_invite_only() && is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument(RED "482 " + channel_name + ": You're not channel operator" RESET);
	if (is_member(input_channel->get_registered(), nickname))
		throw std::invalid_argument(RED "443 " + nickname + ": " + channel_name + ": is already on channel" RESET);
	if (is_invited(input_channel, nickname))
		return ;
	invite_client(*input_client, *input_channel);
}

void Client::handle_mode(const std::string& buffer)
{
	if (!m_is_registered)
		throw std::invalid_argument(RED "451 You have not registered to the server" RESET);

	const std::map<std::string, Channel*>& channels = _server.get_channels();
	std::istringstream	iss(buffer);
	std::string channel_name;
	std::string type;
	std::string mode;
	std::string mode_param;

	iss >> channel_name >> mode >> mode_param;
	if (channel_name.empty() || mode.empty())
		throw std::invalid_argument(RED "461 MODE: Not enough parameters" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 MODE: Too many parameters" RESET);
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument(RED "403 " + channel_name + ": No such channel" RESET);
	if (mode[0] != '+' && mode[0]!= '-')
		throw std::invalid_argument(RED "472 " + mode + ": is not a valid mode" RESET);
	if (is_operator(*input_channel, m_nickname) == false)
		throw std::invalid_argument(RED "482 " + channel_name + ": You're not channel operator" RESET);
	if (mode.empty())
	{
		_server.send_to_client(m_nickname, BLUE "324 " + channel_name + ": modes: " + input_channel->get_modes() + "\r\n" RESET);
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
				throw std::invalid_argument(RED "461 MODE: Not enough parameters" RESET);
			if (input_client == NULL)
				throw std::invalid_argument(RED "406 " + mode_param + ": There was no such nickname" RESET);
			if (is_member(input_channel->get_registered(), mode_param) == false)
				throw std::invalid_argument(RED "441 " + mode_param + ": " + channel_name + " :You aren't on that channel" RESET);
			if (mode[0] == '+')
			{
				if (is_operator(*input_channel, mode_param))
					throw std::invalid_argument(RED "666 " + mode_param + ": " + channel_name + ": is already a channel operator" RESET);
				input_channel->set_operator(mode_param, give);
			}
			else
				input_channel->set_operator(mode_param, take);
			handle_privmsg(input_channel->get_name() + " " + mode_param + " was made " + mode[0] + "o by " + m_nickname);
			break ;
		case ('l'):
			if (mode_param.empty())
				throw std::invalid_argument(RED "461 MODE: Not enough parameters" RESET);
			if (mode[0] == '+')
			{
				std::stringstream ss(mode_param);
				std::stringstream ss_str_limit(mode_param);
				unsigned short limit;
				ss >> limit;
				std::string str_limit;
				ss_str_limit >> str_limit;
				if (limit <= 0)
					throw std::invalid_argument(RED "696 " + str_limit + ": is not a valid limit (must be > 0)" RESET);
				input_channel->set_user_limit(limit);
			}
			else
				input_channel->set_user_limit(0);
			break ;
		default:
			throw std::invalid_argument(RED "472 " + mode + ": is not a valid mode" RESET);
			break ;
	}
}

void Client::kick_parser(const std::string& buffer, std::map<std::string, std::string> &channels_to_nicks) const
{
	std::string							channel_list, nick_list;
	std::istringstream					iss(buffer);
	iss >> channel_list >> nick_list;

	if (channel_list.empty())
		throw std::invalid_argument(RED "461 KICK: Not enough parameters" RESET);
	std::istringstream channel_stream(channel_list);
	std::istringstream nick_stream(nick_list);
	std::string channel, nick;
	while (std::getline(channel_stream, channel, ','))
	{
		if (!std::getline(nick_stream, nick, ','))
			throw std::invalid_argument(RED "461 KICK: Not enough parameters" RESET);
		channels_to_nicks[channel] = nick;
	}
	if (std::getline(nick_stream, nick, ','))
		throw std::invalid_argument(RED "461 KICK: Too many parameters" RESET);
}

void Client::kick_user(std::map<std::string, std::string>& channels_to_nick, const std::map<std::string, Channel*>& channels, const std::map<int, Client*>& clients)
{
	for (std::map<std::string, std::string>::iterator it = channels_to_nick.begin(); it != channels_to_nick.end(); ++it)
	{
		Channel* input_channel = find_channel(channels, it->first);
		if (input_channel == NULL)
			throw std::invalid_argument(RED "403 " + it->first + ": No such channel" RESET);
		Client* input_client = find_client(clients, it->second);
		if (input_client == NULL)
			throw std::invalid_argument(RED "406 " + it->second + ": There was no such nickname" RESET);
		if (!is_member(input_channel->get_registered(), it->second))
			throw std::invalid_argument(RED "441 " + it->second + ": " + it->first + ": They aren't on that channel" RESET);
		if (!is_operator(*input_channel, m_nickname))
			throw std::invalid_argument(RED "482 " + it->first + ": You're not channel operator" RESET);
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
		throw std::invalid_argument(RED "451 You have not registered to the server" RESET);

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
}

void Client::handle_topic(const std::string& buffer)
{
	std::string							channel_name, topic_name;
	std::istringstream					iss(buffer);
	iss >> channel_name >> topic_name;

	if (!m_is_registered)
		throw std::invalid_argument(RED "451 You have not registered to the server" RESET);
	if (are_remain_args(iss))
		throw std::invalid_argument(RED "461 TOPIC: Too many parameters" RESET);
	if (channel_name.empty())
		throw std::invalid_argument(RED "461 TOPIC: Not enough parameters" RESET);
	const std::map<std::string, Channel*>& channels = _server.get_channels();
	Channel* input_channel = find_channel(channels, channel_name);
	if (input_channel == NULL)
		throw std::invalid_argument(RED "403 " + channel_name + ": No such channel" RESET);
	if (topic_name.empty())
	{
		const std::string& topic = input_channel->get_topic();
		if (topic.empty())
			throw std::invalid_argument(RED "331 " + channel_name + ": No topic is set" RESET);
		else
			_server.send_to_client(m_nickname, BLUE "332 " + channel_name + ": topic: " + topic + std::string("\r\n") + "" RESET);
	}
	else if (input_channel->get_topic_restriciton() && !is_operator(*input_channel, m_nickname))
		throw std::invalid_argument(RED "482 " + channel_name + ": You're not channel operator" RESET);
	else if (topic_name == "\"\"" || topic_name == "\'\'")
	{
		handle_privmsg(channel_name + " " + channel_name + ": " + m_nickname + " removed the channel's topic\r\n");
		input_channel->set_topic("");
	}
	else
	{
		handle_privmsg(channel_name + " " + channel_name + ": " + m_nickname + " set new channel topic: " + topic_name + "\r\n");
		input_channel->set_topic(topic_name);
	}
}
