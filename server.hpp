#ifndef SERVER_H
#define SERVER_H

#include "client.hpp"
#include <vector>
#include <string>
#include <netinet/in.h>
#include <poll.h>
#include <map>

class Server {
	private:
		int set_nonblocking(int sockfd);
		void accept_client(int server_fd);
		void handle_client_data(size_t i);

		const int m_port;
		const std::string m_password;
		std::vector<pollfd> m_poll_fds;
		std::map<int, Client*> m_clients; // Change to a map of pointers to Client objects

	public:
		Server(int port, const std::string& password);
		~Server();

		void run();
};

#endif
