
// client.cpp
#include "client.hpp"
#include <sstream>
#include <iostream>

Client::Client(int fd) : m_fd(fd), m_is_registered(false)
{
	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
}

void Client::parse_command(const std::string &command) {
	std::istringstream iss(command);
	std::string cmd;
	iss >> cmd;

	(void) m_fd;
	(void) m_is_registered;

	for (std::map<std::string, CommandHandler>::iterator it = m_commands.begin(); it != m_commands.end(); ++it)
	{
		if (cmd == it->first)
		{
			(this->*(it->second))(command);
			return ;
		}
	}
	std::cout << "Suck a D" << std::endl;
}

void Client::handle_pass(const std::string &command) {
    // Handle PASS command
    // For example, compare with the server's password and return an appropriate response
	std::cout << "PASS" << std::endl;
	(void) command;
}

void Client::handle_nick(const std::string &command) {
    // Handle NICK command
    // For example, update the client's nickname and return an appropriate response
	std::cout << "NICK" << std::endl;
	(void) command;
}

void Client::handle_user(const std::string &command) {
    // Handle USER command
    // For example, update the client's username and realname, and return an appropriate response
	std::cout << "USER" << std::endl;
	(void) command;
}
