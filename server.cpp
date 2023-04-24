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
    for (size_t i = 0; i < m_poll_fds.size(); ++i) {
        close(m_poll_fds[i].fd);
    }
	for (size_t i = 0; i < m_poll_fds.size(); ++i) {
        delete m_clients[m_poll_fds[i].fd]; // Delete the Client object before removing it from the map
        m_clients.erase(m_poll_fds[i].fd);
        close(m_poll_fds[i].fd);
    }
}

int Server::set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error: Unable to get socket flags." << std::endl;
        return -1;
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
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

			m_clients[client_fd] = new Client(client_fd); // Use the 'new' keyword to create a Client object
		}
	}
}

void Server::handle_client_data(size_t i) {
    char buffer[1024];
    ssize_t recv_len = recv(m_poll_fds[i].fd, buffer, sizeof(buffer) - 1, 0);

    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        std::cout << "Received from client (" << m_poll_fds[i].fd << "): " << buffer << std::endl;

		// Use Client::parse_command() to handle the command
        m_clients[m_poll_fds[i].fd]->parse_command(buffer);
    } else if (recv_len == 0) {
        // Client has disconnected
		delete m_clients[m_poll_fds[i].fd];
        m_clients.erase(m_poll_fds[i].fd);
        std::cout << "Client disconnected: " << m_poll_fds[i].fd << std::endl;
        close(m_poll_fds[i].fd);
        m_poll_fds.erase(m_poll_fds.begin() + i);
        --i; // Decrement the index to account for the removed element
    } else {
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
