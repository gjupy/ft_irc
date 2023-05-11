/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gjupy <gjupy@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/04/26 15:48:02 by gjupy             #+#    #+#             */
/*   Updated: 2023/05/11 18:05:32 by gjupy            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <vector>

/*
This file implements the Server class, which is responsible for setting up the listening socket,
accepting client connections, and handling client messages. It uses the poll() function for
asynchronous I/O to manage multiple client connections.
*/

Server::Server(int port, const std::string& password)
	: m_port(port), m_password(password) {}

Server::~Server()
{
	// Clean up
	for (size_t i = 0; i < m_poll_fds.size(); ++i)
		close(m_poll_fds[i].fd);
	for (size_t i = 0; i < m_poll_fds.size(); ++i) {
		delete m_clients[m_poll_fds[i].fd]; // Delete the Client object before removing it from the map
		m_clients.erase(m_poll_fds[i].fd);
		close(m_poll_fds[i].fd);
	}
	for (std::map<std::string, Channel*>::iterator it = m_channel.begin(); it != m_channel.end(); it++)
		delete it->second;
	m_channel.clear();
}

void Server::erase_channel(const std::string& name)
{
	std::cout << "Deleting channel " << name << std::endl;
	for (std::map<std::string, Channel*>::iterator it = m_channel.begin(); it != m_channel.end(); it++)
	{
		if (it->first == name)
		{
			delete it->second;
			m_channel.erase(it);
			return ;
		}
	}
}

Server& Server::operator=(const Server& rhs)
{
	m_port = rhs.m_port;
	m_password = rhs.m_password;
	m_poll_fds = rhs.m_poll_fds;
	m_clients = rhs.m_clients;
	m_channel = rhs.m_channel;
	return (*this);
}

Server::Server(const Server& src)
{
	*this = src;
}

int Server::set_nonblocking(int sockfd) {
	if (fcntl(sockfd, F_SETFL, O_NONBLOCK) == -1) {
		std::cerr << "Error: Unable to set socket to non-blocking mode." << std::endl;
		return -1;
	}

	return 0;
}

void Server::accept_client(int server_fd) {
	sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_addr_len);

	if (client_fd == -1) {
		std::cerr << "Error: Failed to accept client." << std::endl;
	} else {
		std::cout << "Accepted client: " << client_fd << std::endl;

		// Set client socket to non-blocking mode
		if (set_nonblocking(client_fd) == -1) {
			std::cerr << "Error: Failed to set client socket to non-blocking mode." << std::endl;
			close(client_fd);
		} else {
			pollfd client_pollfd;
			client_pollfd.fd = client_fd;
			client_pollfd.events = POLLIN;
			client_pollfd.revents = 0;

			m_poll_fds.push_back(client_pollfd);

			m_clients[client_fd] = new Client(client_fd, *this); // Use the 'new' keyword to create a Client object
		}
	}
}

void Server::handle_client_data(size_t i) {
	char buf[1024];
	ssize_t recv_len = recv(m_poll_fds[i].fd, buf, sizeof(buf) - 1, 0);
	buf[recv_len] = '\0';

	if (recv_len > 0) {
		std::string& buffer = m_clients[m_poll_fds[i].fd]->get_buffer();
		buffer.append(buf);

		// Find the newline character
		size_t newline_pos;
		while ((newline_pos = buffer.find('\n')) != std::string::npos) {
			// Extract command and remove it from the buffer
			std::string command = buffer.substr(0, newline_pos);
			buffer.erase(0, newline_pos + 1);

			// Use Client::parse_command() to handle the command
			m_clients[m_poll_fds[i].fd]->parse_command(command);
		}
	} else if (recv_len == 0) {
		handle_client_disconnection(i);
	} else {
		handle_client_recv_error(i);
	}
}

void Server::handle_client_disconnection(size_t i) {
	delete m_clients[m_poll_fds[i].fd];
	m_clients.erase(m_poll_fds[i].fd);
	std::cout << "Client disconnected: " << m_poll_fds[i].fd << std::endl;
	close(m_poll_fds[i].fd);
	m_poll_fds.erase(m_poll_fds.begin() + i);
	--i; // Decrement the index to account for the removed element
}

void Server::handle_client_recv_error(size_t i) {
	// No data available; non-blocking recv() returns -1 with errno set to EAGAIN or EWOULDBLOCK
	if (errno != EAGAIN && errno != EWOULDBLOCK) {
		delete m_clients[m_poll_fds[i].fd];
		m_clients.erase(m_poll_fds[i].fd);
		std::cerr << "Error: Failed to receive data from client (" << m_poll_fds[i].fd << ")." << std::endl;
		close(m_poll_fds[i].fd);
		m_poll_fds.erase(m_poll_fds.begin() + i);
		--i; // Decrement the index to account for the removed element
	}
}

bool Server::send_to_client(const std::string &target_nickname, const std::string &message)
{
	for (std::map<int, Client*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it) {
		if (it->second->get_nickname() == target_nickname)
		{
			int target_fd = it->first;
			if (send(target_fd, message.c_str(), message.size(), 0) == -1) {
				std::cerr << "Error: Unable to send message to client " << target_fd << std::endl;
				return false;
			}
			return true;
		}
	}
	// Target client not found
	return false;
}

void Server::run() {
	int server_fd;
	struct sockaddr_in server_addr;

	// Create a socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		std::cerr << "Error: Unable to create socket." << std::endl;
		return;
	}

	// Set server socket to non-blocking mode
	if (set_nonblocking(server_fd) == -1) {
		close(server_fd);
		return;
	}

	// Prepare the sockaddr_in structure
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(m_port);

	// Bind the socket
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		std::cerr << "Error: Unable to bind socket." << std::endl;
		close(server_fd);
		return;
	}

	// Start listening
	if (listen(server_fd, 10) == -1) {
		std::cerr << "Error: Unable to listen on socket." << std::endl;
		close(server_fd);
		return;
	}

	std::cout << "Server listening on port " << m_port << std::endl;

	// Initialize pollfd structures for server and clients

	pollfd server_pollfd;
	server_pollfd.fd = server_fd;
	server_pollfd.events = POLLIN;
	server_pollfd.revents = 0;

	m_poll_fds.push_back(server_pollfd);

	// Main loop
	while (true) {
		int poll_result = poll(&m_poll_fds[0], m_poll_fds.size(), -1);

		if (poll_result == -1) {
			std::cerr << "Error: Poll failed." << std::endl;
			break;
		}

		for (size_t i = 0; i < m_poll_fds.size(); ++i) {
			if (m_poll_fds[i].revents & POLLIN) {
				if (m_poll_fds[i].fd == server_fd) {
					// New client connection
					accept_client(server_fd);
				} else {
					// Client data available
					handle_client_data(i);
				}
			}
		}
	}
}

const std::string	Server::get_password() const
{
	return (m_password);
}


const std::map<std::string, Channel*>&	Server::get_channels() const
{
	return (m_channel);
}

void	Server::add_new_channel(Channel* new_channel)
{
	m_channel[new_channel->get_name()] = new_channel;
}

const std::map<int, Client*>&	Server::get_clients() const
{
	return (m_clients);
}
