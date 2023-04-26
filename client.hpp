// client.hpp
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <map>

class Server;

class Client {
	private:
		int			m_fd;
		bool		m_is_registered;
		std::string	m_nickname;
		std::string	m_username;
		Server&		_server;

		void handle_pass(const std::string&);
		void handle_nick(const std::string&);
		void handle_user(const std::string&);

		// void handle_join(const std::string&);
		// void handle_kick(const std::string&);
		// void handle_topic(const std::string&);
		// void handle_invite(const std::string&);
		// void handle_mode(const std::string&);

		void handle_privmsg(const std::string& arg);

		typedef void (Client::*CommandHandler)(const std::string&);
		std::map <std::string, CommandHandler> m_commands;

	public:
		Client(int fd, Server& server);

		const std::string& get_nickname() const;

		void parse_command(const std::string &command);

};

#include "server.hpp"

#endif
