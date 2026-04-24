#ifndef CONNECTION_H
#define CONNECTION_H


//quick fix to avoid including <netinet/in.h> in main.c just for sockaddr_in
struct sockaddr_in;

int server_socket_create(int port, int backlog);
void connection_handle(int client_fd, struct sockaddr_in *client_addr);
void server_accept_loop(int server_fd);

#endif /* CONNECTION_H */

