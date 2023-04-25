
// client.cpp
#include "client.hpp"
#include <sstream>
#include <iostream>

Client::Client(int fd, const std::string &command) : m_fd(fd), m_is_registered(false), m_password(command)
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

	for (std::map<std::string, CommandHandler>::iterator it = m_commands.begin(); it != m_commands.end(); ++it)
	{
		if (cmd == it->first)
		{
			std::string arg;
			iss >> arg;
			(this->*(it->second))(arg);
			return ;
		}
	}
	std::cout << "Suck a D" << std::endl;
}

void Client::handle_pass(const std::string &arg) {
	if (m_is_registered) {
		return;
	}

	if (arg != m_password) {
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
