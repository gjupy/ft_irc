// client.hpp
#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <map>

class Client {
	private:
		int m_fd;
		bool m_is_registered;
		const std::string& m_password;
		std::string m_nickname;
		std::string m_username;

		void handle_pass(const std::string&);
		void handle_nick(const std::string&);
		void handle_user(const std::string&);

		typedef void (Client::*CommandHandler)(const std::string&);
		std::map <std::string, CommandHandler> m_commands;
	public:
		Client(int fd, const std::string& password);

		void parse_command(const std::string &command);

};

#endif
