#include "./connection_test.h"
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

void test_function_server_socket_create(void){
    // Create a valid server socket with dynamic port
    int fd = server_socket_create(0, 5);  // port 0 = OS picks available port
    TEST_ASSERT_GREATER_THAN_INT(-1, fd);
    close(fd);

    // Create another socket to verify we can create multiple
    int fd2 = server_socket_create(0, 10);
    TEST_ASSERT_GREATER_THAN_INT(-1, fd2);
    close(fd2);

    // Different backlog values
    int fd3 = server_socket_create(0, 1);
    TEST_ASSERT_GREATER_THAN_INT(-1, fd3);
    close(fd3);
}

// void connection_handle(int client_fd, struct sockaddr_in *client_addr);
void test_function_connection_handle(void){
    // connection_handle is an infinite loop that cant be properly tested without threading
    
    // Create a socket pair to simulate client-server connection
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        TEST_FAIL();
        return;
    }

    int server_sock = sockets[0];
    int client_sock = sockets[1];
    
    // Prepare a client address struct
    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    client_addr.sin_port = htons(8080);
    
    // Send a simple HTTP GET request from client side
    const char *request = "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    if (write(client_sock, request, strlen(request)) < 0) {
        TEST_FAIL();
        close(server_sock);
        close(client_sock);
        return;
    }

    // Close write end to signal end of request
    shutdown(client_sock, SHUT_WR);

    close(server_sock);
    close(client_sock);
}

// void server_accept_loop(int server_fd);
void test_function_server_accept_loop(void){
    // server_accept_loop is an infinite blocking loop, therefore cant be tested without threading    

    // Create a server socket
    int server_fd = server_socket_create(0, 5);
    if (server_fd < 0) {
        TEST_FAIL();
        return;
    }
    
    // Verify the socket is valid (can get socket name)
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int result = getsockname(server_fd, (struct sockaddr *)&addr, &len);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(AF_INET, addr.sin_family);
    
    
    close(server_fd);
}

void setUp(void) {
    // Not needed
}

void tearDown(void) {
    // Not needed
}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_function_server_socket_create);
    RUN_TEST(test_function_connection_handle);
    RUN_TEST(test_function_server_accept_loop);
    return UNITY_END();
}