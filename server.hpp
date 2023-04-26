/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:19 by gjupy             #+#    #+#             */
/*   Updated: 2023/04/26 18:42:37 by gjupy            ###   ########.fr       */
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

		void run();
};

#include "client.hpp"

#endif
