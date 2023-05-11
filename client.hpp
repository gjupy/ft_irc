// client.hpp
#ifndef CLIENT_H
#define CLIENT_H

# define PURPLE "\033[0;35m"
# define GREEN "\033[0;32m"
# define RESET "\033[0m"
# define RED "\033[31m"

#include <string>
#include <map>
#include "Channel.hpp"
#include <set>

class Channel;
class Server;

class Client {
	private:
		int			m_fd;
		bool		m_is_registered;
		Server&		_server;
		std::string	m_nickname;
		std::string	m_username;
		bool m_authenticated;
		std::string buffer;
		static int unregistered_count;

		void handle_pass(const std::string&);
		void handle_nick(const std::string&);

		void handle_user(const std::string&);
		bool username_exists(const std::string &username) const;
		bool nickname_exists(const std::string &nickname) const;

		void handle_join(const std::string&);
		void join_parser(const std::string&, std::map<std::string, std::string> &);
		bool is_valid_key(Channel*, const std::string&);
		bool is_invited(const Channel*, const std::string&);
		bool is_registered_channel(const Channel*);
		void add_user(std::map<std::string, std::string>&);

		void handle_invite(const std::string&);
		Client* find_client(const std::map<int, Client*>&, const std::string&) const;
		Channel* find_channel(const std::map<std::string, Channel*>&, const std::string& ) const;
		bool is_operator(Channel&, const std::string&) const;
		bool is_member(const std::set<Client*>&, const std::string&) const;
		void invite_client(Client&, Channel&);

		void handle_mode(const std::string&);

		void handle_kick(const std::string&);
		void kick_parser(const std::string&, std::map<std::string, std::string>&) const;
		void kick_user(std::map<std::string, std::string>&, const std::map<std::string, Channel*>&, const std::map<int, Client*>&);

		void handle_topic(const std::string&);
		void handle_privmsg(const std::string&);
		void handle_exit(const std::string&);

		typedef void (Client::*CommandHandler)(const std::string&);
		std::map <std::string, CommandHandler> m_commands;

	public:
		Client(int fd, Server& server);
		Client(const Client& src);
		Client& operator=(const Client& rhs);

		const std::string& get_nickname() const;
		const std::string& get_username() const;
		std::string& get_buffer();

		void parse_command(const std::string &command);

};

#include "server.hpp"

#endif
