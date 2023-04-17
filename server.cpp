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

const int PORT = 12345;

int set_nonblocking(int sockfd) {
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

void accept_client(int server_fd, std::vector<pollfd>& poll_fds) {
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

            poll_fds.push_back(client_pollfd);
        }
    }
}

void handle_client_data(pollfd& client_pollfd, std::vector<pollfd>& poll_fds, size_t i) {
    char buffer[1024];
    ssize_t recv_len = recv(client_pollfd.fd, buffer, sizeof(buffer) - 1, 0);

    if (recv_len > 0) {
        buffer[recv_len] = '\0';
        std::cout << "Received from client (" << client_pollfd.fd << "): " << buffer << std::endl;
    } else if (recv_len == 0) {
        // Client has disconnected
        std::cout << "Client disconnected: " << client_pollfd.fd << std::endl;
        close(client_pollfd.fd);
        poll_fds.erase(poll_fds.begin() + i);
        --i; // Decrement the index to account for the removed element
    } else {
        // No data available; non-blocking recv() returns -1 with errno set to EAGAIN or EWOULDBLOCK
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "Error: Failed to receive data from client (" << client_pollfd.fd << ")." << std::endl;
            close(client_pollfd.fd);
            poll_fds.erase(poll_fds.begin() + i);
            --i; // Decrement the index to account for the removed element
        }
    }
}

void run_server() {
	int server_fd;
	struct sockaddr_in server_addr;

	// Create a socket
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1) {
		std::cerr << "Error: Unable to create socket." << std::endl;
		exit(EXIT_FAILURE);
	}

	// Set the server socket to non-blocking mode
	if (set_nonblocking(server_fd) == -1) {
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Configure the server address
	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	// Bind the socket to the server address
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		std::cerr << "Error: Unable to bind socket to address." << std::endl;
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	// Listen for incoming connections
	if (listen(server_fd, 5) == -1) {
		std::cerr << "Error: Unable to listen for incoming connections." << std::endl;
		close(server_fd);
		exit(EXIT_FAILURE);
	}

	std::cout << "Server is listening on port " << PORT << "..." << std::endl;

	std::vector<pollfd> poll_fds;
	pollfd server_pollfd = {server_fd, POLLIN, 0};
	poll_fds.push_back(server_pollfd);

    while (true) {
        int poll_ret = poll(poll_fds.data(), poll_fds.size(), -1);

        if (poll_ret > 0) {
            for (size_t i = 0; i < poll_fds.size(); ++i) {
                if (poll_fds[i].revents & POLLIN) {
                    if (poll_fds[i].fd == server_fd) {
                        accept_client(server_fd, poll_fds);
                    } else {
                        handle_client_data(poll_fds[i], poll_fds, i);
                    }
                }
            }
        } else if (poll_ret == -1) {
            std::cerr << "Error: poll() failed." << std::endl;
            break;
        }
    }

    // Close all open sockets
    for (size_t i = 0; i < poll_fds.size(); ++i) {
        close(poll_fds[i].fd);
    }
}
