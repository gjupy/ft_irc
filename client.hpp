// client.hpp
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <map>
#include "Channel.hpp"

class Channel;
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

		void handle_join(const std::string&);
		void join_parser(const std::string&, std::map<std::string, std::string> &);
		bool is_valid_key(Channel* channel, const std::string& input_key);
		bool is_invited(const Channel* channel);
		bool is_registered(const Channel* channel);
		void add_user(std::map<std::string, std::string> &);

		void handle_invite(const std::string&);

		// void handle_kick(const std::string&);
		// void handle_topic(const std::string&);
		// void handle_mode(const std::string&);

		typedef void (Client::*CommandHandler)(const std::string&);
		std::map <std::string, CommandHandler> m_commands;

	public:
		Client(int fd, Server& server);
		Client(const Client& src);
		Client& operator=(const Client& rhs);

		void parse_command(const std::string &command);

		const std::string	get_nickname() const;
		const std::string	get_username() const;

};

#include "server.hpp"

#endif
