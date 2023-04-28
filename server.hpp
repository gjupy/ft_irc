/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cboubour <cboubour@student.42heilbronn.    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:47:19 by gjupy             #+#    #+#             */
/*   Updated: 2023/04/28 15:34:35 by cboubour         ###   ########.fr       */
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

		const std::string get_password() const;
		const std::map<std::string, Channel*>& get_channels() const;

		bool send_to_client(const std::string &target_nickname, const std::string &message);

		void run();

		void add_new_channel(Channel*);
};

#include "client.hpp"

#endif
