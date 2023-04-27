
// client.cpp
#include "client.hpp"
#include <sstream>
#include <iostream>

Client::Client(int fd, Server& server) : m_fd(fd), m_is_registered(false), _server(server), m_nickname(""), m_username(""), buffer("")
{
	m_commands["PASS"] = &Client::handle_pass;
	m_commands["NICK"] = &Client::handle_nick;
	m_commands["USER"] = &Client::handle_user;
	// m_commands["JOIN"] = &Client::handle_join;
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
	std::cout << "Suck a D" << std::endl;
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
	std::cout << "Client " << m_fd << " registered" << std::endl;

}

void Client::handle_user(const std::string &args) {
	if (!m_authenticated){
		std::cout << "Error: USER command requires an authenticated client." << std::endl;
		return;
	}

	std::istringstream iss(args);
	std::string username;

	if (!(iss >> username)) {
		std::cout << "Error: USER command requires a username" << std::endl;
		return;
	}
	m_username = username;

	std::cout << "Client " << m_fd << " set username to: " << m_username << std::endl;
}

void Client::handle_nick(const std::string &args) {
	if (!m_authenticated || m_username.empty()){
		std::cout << "Error: NICK command requires an authenticated client with username." << std::endl;
		return ;
	}

	std::string arg;
	std::istringstream iss(args);
	iss >> arg;

	if (arg.empty()) {
		std::cout << "Error: NICK command requires an argument." << std::endl;
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

	std::istringstream iss(args);
	std::string target_nickname, text;

	if (!(iss >> target_nickname)) {
		std::cout << "Error: PRIVMSG command requires a target nickname" << std::endl;
		return;
	}

	getline(iss, text);
	if (text.empty() || text[0] != ' ') {
		std::cout << "Error: PRIVMSG command requires a message text" << std::endl;
		return;
	}

	text = text.substr(1); // Remove the space at the beginning

	// Remove the newline character if present
	if (!text.empty() && text[text.size() - 1] == '\n') {
		text.resize(text.size() - 1);
	}

	std::string message = m_nickname + ": " + text + "\n";

	if (!_server.send_to_client(target_nickname, message)) {
		std::cout << "Error: Failed to send message to " << target_nickname << std::endl;
	}

}
