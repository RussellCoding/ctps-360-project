#include "router_test.h"
#include <stdlib.h>

void test_function_router_dispatch(void){
    HttpRequest req;
    char in[256];
    http_parse_request(raw_get, strlen(raw_get), &req);
    router_dispatch(&req, in, 256);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1 200 OK\r\n"
        "Content-Length: 138\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html><head><title>Hello</title></head>\n"
        "<body><h1>Welcome to MyHTTPServer</h1><p>The server is running.</p></body></html>\n", in);

    http_parse_request(raw_get_full_headers, strlen(raw_get_full_headers), &req);
    strcpy(req.path, "/headers");

    router_dispatch(&req, in, 256);
    TEST_ASSERT_EQUAL_STRING("HTTP/1.1 200 OK\r\n"
        "Content-Length: 164\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Method: GET\n"
        "Path: /headers\n\n"
        "Host: www.example.com\n"
        "User-Agent: curl/7.68.0\n"
        "Accept: */*\n"
        "Accept-Encoding: gzip, deflate\n"
        "Connection: keep-alive\n"
        "Cache-Control: no-cache\n", in);
}

void test_function_handle_index(void){
    HttpRequest req;
    char in[256];
    http_parse_request(raw_get, strlen(raw_get), &req);
    handle_index(&req, in, sizeof(in));
    
    // Check for status code in response
    TEST_ASSERT(strstr(in, "HTTP/1.1 200 OK") != NULL);
    // Check for expected body
    TEST_ASSERT(strstr(in, "Welcome to MyHTTPServer") != NULL);
    // Check for content type header
    TEST_ASSERT(strstr(in, "text/html") != NULL);

}

void test_function_handle_echo(void){
    HttpRequest req;
    char in[256];
    
    strcpy(req.path, "/echo");
    strcpy(req.method, "GET");
    handle_echo(&req, in, 256);
    // Check for HTTP 200 response
    TEST_ASSERT(strstr(in, "HTTP/1.1 200 OK") != NULL);
    // Check for response body
    TEST_ASSERT_NOT_NULL(strstr(in, "(no query string)")); 

    // Test with request body
    strcpy(req.path, "/echo");
    strcpy(req.raw_path, "/echo?test+message");
    strcpy(req.method, "GET");
    handle_echo(&req, in, 256);
    TEST_ASSERT(strstr(in, "test message") != NULL);
}

void test_function_handle_headers(void){
    HttpHeader h1;
    // Add values to the header
    strcpy(h1.name, "Test-Header");
    strcpy(h1.value, "test-value");

    // Add the header to the request
    HttpRequest req;
    req.headers[0]=h1;
    req.header_count=1;
    char in[256];

    strcpy(req.path, "/headers");
    handle_headers(&req, in, 256);
    // Check for HTTP 200 response
    TEST_ASSERT(strstr(in, "HTTP/1.1 200 OK") != NULL);
    // Check for header in response
    TEST_ASSERT_NOT_NULL(strstr(in, "Test-Header: test-value"));
}

void test_function_handle_health(void){
    HttpRequest req;
    char in[256];
    strcpy(req.path, "/health");
    handle_health(&req, in, 256);

    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 200 OK"));
    TEST_ASSERT_NOT_NULL(strstr(in, "\"status\""));
}

void test_function_handle_post_echo(void){
    HttpRequest req;
    req.body=malloc(1024);
    char in[256];
    strcpy(req.method, "POST");
    strcpy(req.body, "test");
    req.body_length=4;
    handle_post_echo(&req, in, 256);
    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 200 OK"));
    TEST_ASSERT_NOT_NULL(strstr(in, "test"));
    
    
    strcpy(req.body, "test message");
    req.body_length=4;
    handle_post_echo(&req, in, 256);
    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 200 OK"));
    TEST_ASSERT( (strstr(in, "test") && !strstr(in, "message"))==1 );


    
}

void test_function_handle_not_found(void){
    HttpRequest req;
    char in[256];
    handle_not_found(&req, in, sizeof(in));
    
    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 404 Not Found"));
}

void test_function_handle_method_not_allowed(void){
    HttpRequest req;
    char in[256];
    handle_method_not_allowed(&req, in, 256);

    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 405 Method Not Allowed"));
}

void test_function_handle_bad_request(void){
    HttpRequest req;
    char in[256];
    handle_bad_request(&req, in, 256);

    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 400 Bad Request"));
}

void test_function_handle_internal_error(void){
    HttpRequest req;
    char in[256];
    handle_internal_error(&req, in, 256);

    TEST_ASSERT_NOT_NULL(strstr(in, "HTTP/1.1 500 Internal Server Error"));
}
void setUp(void){
    // None needed
}
void tearDown(void){
    // None needed
}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_function_router_dispatch);
    RUN_TEST(test_function_handle_index);
    RUN_TEST(test_function_handle_echo);
    RUN_TEST(test_function_handle_headers);
    RUN_TEST(test_function_handle_health);
    RUN_TEST(test_function_handle_post_echo);
    RUN_TEST(test_function_handle_not_found);
    RUN_TEST(test_function_handle_method_not_allowed);
    RUN_TEST(test_function_handle_bad_request);
    RUN_TEST(test_function_handle_internal_error);
    return UNITY_END();
}