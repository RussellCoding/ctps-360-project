#ifndef CONNECTION_TEST_H
#define CONNECTION_TEST_H
#include <string.h>
#include "../socket/connection.h"
#include "unity/unity.h"


/* socket/connection.h */

// int server_socket_create(int port, int backlog);
void test_function_server_socket_create(void);

// void connection_handle(int client_fd, struct sockaddr_in *client_addr);
void test_function_connection_handle(void);

// void server_accept_loop(int server_fd);
void test_function_server_accept_loop(void);

#endif /* CONNECTION_TEST_H */
