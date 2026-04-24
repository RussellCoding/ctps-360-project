#include "./parser_test.h"

// ParseResult http_parse_request(const char *buf, size_t buf_len, HttpRequest *req);
void test_function_http_parse_request(void) {
    HttpRequest req;
    
    // Test basic GET request
    ParseResult res = http_parse_request(raw_get, strlen(raw_get), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    TEST_ASSERT_EQUAL_STRING("GET", req.method);
    TEST_ASSERT_EQUAL_STRING("/", req.path);
    TEST_ASSERT_EQUAL_STRING("1.1", req.version);
    TEST_ASSERT_EQUAL_INT(2, req.header_count); // Host and Connection
    TEST_ASSERT_EQUAL_STRING("Host", req.headers[0].name);
    TEST_ASSERT_EQUAL_STRING("example.com", req.headers[0].value);
    TEST_ASSERT_EQUAL_STRING("Connection", req.headers[1].name);
    TEST_ASSERT_EQUAL_STRING("close", req.headers[1].value);
    TEST_ASSERT_EQUAL_INT(0, req.keep_alive); // Connection: close
    http_request_free(&req);
    
    // Test GET with query string
    res = http_parse_request(raw_get_with_query, strlen(raw_get_with_query), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    TEST_ASSERT_EQUAL_STRING("GET", req.method);
    TEST_ASSERT_EQUAL_STRING("/api/users", req.path); // Should strip query
    TEST_ASSERT_EQUAL_STRING("/api/users?id=123&name=john", req.raw_path); // Raw path with query
    TEST_ASSERT_EQUAL_INT(2, req.query_param_count); // id and name
    TEST_ASSERT_EQUAL_STRING("id", req.query_params[0].key);
    TEST_ASSERT_EQUAL_STRING("123", req.query_params[0].value);
    TEST_ASSERT_EQUAL_STRING("name", req.query_params[1].key);
    TEST_ASSERT_EQUAL_STRING("john", req.query_params[1].value);
    TEST_ASSERT_EQUAL_INT(1, req.keep_alive); // Connection: keep-alive
    http_request_free(&req);
    
    // Test GET with multiple headers
    res = http_parse_request(raw_get_full_headers, strlen(raw_get_full_headers), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    TEST_ASSERT_EQUAL_STRING("GET", req.method);
    TEST_ASSERT_EQUAL_STRING("/index.html", req.path);
    TEST_ASSERT_EQUAL_INT(6, req.header_count); // All headers
    // Test that we can find specific headers
    const char *user_agent = http_get_header(&req, "User-Agent");
    TEST_ASSERT_NOT_NULL(user_agent);
    TEST_ASSERT_EQUAL_STRING("curl/7.68.0", user_agent);
    const char *accept = http_get_header(&req, "Accept");
    TEST_ASSERT_NOT_NULL(accept);
    TEST_ASSERT_EQUAL_STRING("*/*", accept);
    http_request_free(&req);
    
    // Test HTTP/1.0
    res = http_parse_request(raw_get_http10, strlen(raw_get_http10), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    TEST_ASSERT_EQUAL_STRING("1.0", req.version);
    http_request_free(&req);
    
    // Test error cases
    const char *malformed_request = "INVALID REQUEST\r\n\r\n";
    res = http_parse_request(malformed_request, strlen(malformed_request), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_BAD_REQUEST, res);
    
    // Test incomplete request
    const char *incomplete_request = "GET / HTTP/1.1\r\nHost: example.com\r\n"; // Missing final \r\n
    res = http_parse_request(incomplete_request, strlen(incomplete_request), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_ERR_INCOMPLETE, res);
}

// void http_request_free(HttpRequest *req);
void test_function_http_request_free(void){
    HttpRequest req;
    ParseResult res = http_parse_request(raw_get, strlen(raw_get), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    
    // Body should be NULL in a GET request
    TEST_ASSERT_NULL(req.body); 
    
    // Free should not crash and should clean up properly
    http_request_free(&req);
    
    // After free, body should be NULL and lengths should be 0
    TEST_ASSERT_NULL(req.body);
    TEST_ASSERT_EQUAL_INT(0, req.body_length);
}

// const char *http_get_header(const HttpRequest *req, const char *name);
void test_function_http_get_header(void){
    HttpRequest req;
    ParseResult res = http_parse_request(raw_get_full_headers, strlen(raw_get_full_headers), &req);
    TEST_ASSERT_EQUAL_INT(PARSE_OK, res);
    
    // Test finding existing headers (case-insensitive)
    const char *host = http_get_header(&req, "Host");
    TEST_ASSERT_NOT_NULL(host);
    TEST_ASSERT_EQUAL_STRING("www.example.com", host);
    
    const char *user_agent = http_get_header(&req, "user-agent"); // lowercase
    TEST_ASSERT_NOT_NULL(user_agent);
    TEST_ASSERT_EQUAL_STRING("curl/7.68.0", user_agent);
    
    const char *accept = http_get_header(&req, "ACCEPT"); // uppercase
    TEST_ASSERT_NOT_NULL(accept);
    TEST_ASSERT_EQUAL_STRING("*/*", accept);
    
    // Test non-existent header
    const char *not_found = http_get_header(&req, "X-Custom-Header");
    TEST_ASSERT_NULL(not_found);
    
    http_request_free(&req);
}

// const char *http_parse_error_status(ParseResult result);
void test_function_http_parse_error_status(void){
 
    const char* status = http_parse_error_status(PARSE_ERR_BAD_REQUEST);
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING("400 Bad Request", status);
    
    status = http_parse_error_status(PARSE_ERR_URI_TOO_LONG);
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING("414 URI Too Long", status);
    
    status = http_parse_error_status(PARSE_ERR_NO_HOST);
    TEST_ASSERT_NOT_NULL(status);
    TEST_ASSERT_EQUAL_STRING("400 Bad Request", status);
}

void setUp(void) {
    // No set up needed
}

void tearDown(void) {
    // No tear down needed
}

int main(){
    UNITY_BEGIN();
    RUN_TEST(test_function_http_parse_request);
    RUN_TEST(test_function_http_request_free);
    RUN_TEST(test_function_http_get_header);
    RUN_TEST(test_function_http_parse_error_status);
    return UNITY_END();
}