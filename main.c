#include <stdio.h>
#include <stdlib.h>
#include "socket/connection.h"

// Define the default port number for the server to listen on
#define DEFAULT_PORT    8080
// max number of pending connections in the queue for accept()
#define BACKLOG         16

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;

    // check if a port number is provided as a command-line argument and validate it
    if (argc == 2) {
        port = atoi(argv[1]);
        // Validate the port number to ensure it's within the valid range (1-65535)
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            return 1;
        }
    }

    // Create a server socket and bind it to the specified port
    int server_fd = server_socket_create(port, BACKLOG);
    if (server_fd < 0) return 1;

    // Start the main loop to accept and handle incoming client connections
    server_accept_loop(server_fd);

    return 0;
}