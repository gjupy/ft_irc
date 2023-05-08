/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:19 by gjupy             #+#    #+#             */
/*   Updated: 2023/05/05 17:58:52 by gjupy            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_H
#define SERVER_H

#include "Channel.hpp"
#include <vector>
#include <string>
#include <netinet/in.h>
#include <poll.h>
#include <map>

// Names the Server for response-generation
#define SERVERNAME "idiotic.chat"

// Names the Host for response-generation
#define HOST "localhost"

class Client;
class Channel;

class Server {

	private:
		int set_nonblocking(int sockfd);
		void accept_client(int server_fd);

		void handle_client_data(size_t i);
		void handle_client_disconnection(size_t i);
		void handle_client_recv_error(size_t i);

		int								m_port;
		std::string						m_password;
		std::vector<pollfd>				m_poll_fds;

		std::map<int, Client*>			m_clients;
		std::map<std::string, Channel*>	m_channel;

	public:
		Server(int port, const std::string& password);
		Server(const Server& src);
		Server& operator=(const Server& rhs);
		~Server();

		const std::string						get_password() const;
		const std::map<std::string, Channel*>&	get_channels() const;
		const std::map<int, Client*>&			get_clients() const;

		bool send_to_client(const std::string &target_nickname, const std::string &message);

		void run();

		void add_new_channel(Channel*);
};

#include "client.hpp"

#endif
