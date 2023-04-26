/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cboubour <cboubour@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:19 by gjupy             #+#    #+#             */
/*   Updated: 2023/04/26 20:42:10 by cboubour         ###   ########.fr       */
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

class Client;
class Channel;

class Server {

	private:
		int set_nonblocking(int sockfd);
		void accept_client(int server_fd);
		void handle_client_data(size_t i);

		const int								m_port;
		const std::string						m_password;
		std::vector<pollfd>						m_poll_fds;

		std::map<int, Client*>			m_clients;
		std::map<std::string, Channel*>	m_channel;

	public:
		Server(int port, const std::string& password);
		~Server();

		const std::string get_password() const;

		bool send_to_client(const std::string &target_nickname, const std::string &message);

		void run();
};

#include "client.hpp"

#endif
