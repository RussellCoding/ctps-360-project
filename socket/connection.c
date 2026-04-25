#include "connection.h"
#include "../parser/http_parser.h"
#include "../parser/request.h"
#include "../router/router.h"
#include "../router/handlers.h"
#include "../response/response_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define READ_BUF_SIZE   (8192)
//#define WRITE_BUF_SIZE  (RESPONSE_TOTAL_MAX) this is not defined
#define WRITE_BUF_SIZE  (READ_BUF_SIZE)

/*
this creates, binds, and listens to the tcp socket
    port: port number to bind to
    backlong: max length of queue of pending connections 
    return: will return -1 on failure
*/
int server_socket_create(int port, int backlog) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    //faster response time and not waiting 
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_REUSEADDR");
        close(fd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons((uint16_t)port);
    //binds the socket to the adress and port
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }
    //listens for incoming
    if (listen(fd, backlog) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    printf("[server] Listening on port %d\n", port);
    return fd;
}


/*
    reads bytes from client until end of http header
    fd: soket file discrip
    buf: store received data
    buf_size: max size of the buffer
    return: total bytes read, or -1 on closure or failure 
*/
static int recv_request(int fd, char *buf, int buf_size) {
    int total = 0;

    while (total < buf_size - 1) {
        int n = (int)recv(fd, buf + total, buf_size - total - 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;   // retry if intrupted
            perror("recv");
            return -1;
        }
        if (n == 0) return total;   //client closed

        total += n;
        buf[total] = '\0';//null term for string funcs 

        //stops reading once end, logic for reading the body  is handled sep
        if (strstr(buf, "\r\n\r\n") || strstr(buf, "\n\n")) break;
    }
    return total;
}


/*
    makes sure all bytes in the buffer are successfully written to the socket
    fd: file descrip
    buf: data that will be sent
    len: the lenght of the data
    reutrn: 0 on success and -1 on failure
*/
static int send_all(int fd, const char *buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = (int)send(fd, buf + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue; //retry on interruption
            perror("send");
            return -1;
        }
        sent += n;
    }
    return 0;
}


/*
    this will manage the lifecycle of a single client connection
    this is "keep alive"
    client_fd: this is the socket fo the connected client
    client_addr: data about the clients address 
*/
void connection_handle(int client_fd, struct sockaddr_in *client_addr) {
    char peer_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr->sin_addr, peer_ip, sizeof(peer_ip));
    fprintf(stderr, "[conn] accepted from %s:%d\n",
            peer_ip, ntohs(client_addr->sin_port));
    //allocates mem for io buffers
    char *read_buf  = malloc(READ_BUF_SIZE);
    char *write_buf = malloc(WRITE_BUF_SIZE);

    if (!read_buf || !write_buf) {
        fprintf(stderr, "[conn] malloc failed\n");
        free(read_buf);
        free(write_buf);
        close(client_fd);
        return;
    }
    //loop to support multiple requests during a single connection aka the keep alive
    while (1) {
        memset(read_buf,  0, READ_BUF_SIZE);
        memset(write_buf, 0, WRITE_BUF_SIZE);

        int nread = recv_request(client_fd, read_buf, READ_BUF_SIZE);
        if (nread <= 0) break;   //end loop if disconnects

        HttpRequest req;
        ParseResult result = http_parse_request(read_buf, (size_t)nread, &req);

        int response_len;

        if (result == PARSE_OK) {
            fprintf(stderr, "[req]  %s %s %s\n",
                    req.method, req.path, req.version);
            router_dispatch(&req, write_buf, WRITE_BUF_SIZE);
            response_len = (int)strlen(write_buf);
        } else {
            //400 bad request
            fprintf(stderr, "[req]  parse error: %s\n",
                    http_parse_error_status(result));
            //response_len = response_build_error(400, 0, write_buf, WRITE_BUF_SIZE); this was giving an error
            const char *res_str = http_parse_error_status(PARSE_ERR_BAD_REQUEST);
            response_len = strlen(res_str);
            strncpy(write_buf, res_str, WRITE_BUF_SIZE);
        }

        //send response back to client
        if (response_len > 0) {
            if (send_all(client_fd, write_buf, response_len) < 0) break;
        }

        /* req may own heap memory (body) */
        http_request_free(&req);

        //eend connection
        if (!req.keep_alive) break;
    }

    fprintf(stderr, "[conn] closing %s\n", peer_ip);
    free(read_buf);
    free(write_buf);
    close(client_fd);
}


/*
    main server loop that blocks and waits for new client connections 
    this is single-threaded
    server_fd: listening to server socket
*/
void server_accept_loop(int server_fd) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        //block until new client connects
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &addr_len);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept");
            break;
        }
        //handle connection 
        connection_handle(client_fd, &client_addr);
    }
}