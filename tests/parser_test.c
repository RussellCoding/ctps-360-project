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

//int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len);
void test_function_url_decode(void){
    char dst[256];
    
    // Test simple string
    int result = url_decode(dst, sizeof(dst), "hello", 5);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("hello", dst);
    
    // Test percent encoding
    result = url_decode(dst, sizeof(dst), "hello%20world", 13);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("hello world", dst);
    
    // Test plus as space
    result = url_decode(dst, sizeof(dst), "hello+world", 11);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("hello world", dst);
    
    // Test hex uppercase
    result = url_decode(dst, sizeof(dst), "test%2F%3A", 10);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("test/:", dst);
    
    // Test hex lowercase
    result = url_decode(dst, sizeof(dst), "test%2f%3a", 10);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("test/:", dst);
    
    // Test mixed encoding
    result = url_decode(dst, sizeof(dst), "hello%20world%2BTest+123", 25);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("hello world+Test 123", dst);
    
    // Test truncated escape
    result = url_decode(dst, sizeof(dst), "hello%2", 7);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test invalid hex
    result = url_decode(dst, sizeof(dst), "hello%ZZ", 8);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test buffer too small
    result = url_decode(dst, 5, "hello world", 11);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test empty string
    result = url_decode(dst, sizeof(dst), "", 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("", dst);
}

//int path_normalize(char *path);
void test_function_path_normalize(void){
    char path[MAX_PATH];
    int result;
    
    // Test simple path
    strcpy(path, "/hello/world");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/hello/world", path);
    
    // Test dot segment
    strcpy(path, "/hello/./world");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/hello/world", path);
    
    // Test double-dot
    strcpy(path, "/hello/world/../test");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/hello/test", path);
    
    // Test multiple dots
    strcpy(path, "/a/b/c/../../d");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/a/d", path);
    
    // Test root
    strcpy(path, "/");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/", path);
    
    // Test escape attempt
    strcpy(path, "/../etc/passwd");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test escape attempt deep
    strcpy(path, "/a/b/../../../../etc/passwd");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test consecutive slashes
    strcpy(path, "/hello//world");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("/hello/world", path);
    
    // Test not absolute
    strcpy(path, "hello/world");
    result = path_normalize(path);
    TEST_ASSERT_EQUAL_INT(-1, result);
    
    // Test NULL
    result = path_normalize(NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// int parse_query_string(const char *qs, size_t len, QueryParam *params, int max_params);
void test_function_parse_query_string(void){
    QueryParam params[MAX_QUERY_PARAMS];
    int result;
    
    // Test single param
    result = parse_query_string("key=value", 9, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING("key", params[0].key);
    TEST_ASSERT_EQUAL_STRING("value", params[0].value);
    
    // Test multiple params
    result = parse_query_string("key1=value1&key2=value2&key3=value3", 36, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(3, result);
    TEST_ASSERT_EQUAL_STRING("key1", params[0].key);
    TEST_ASSERT_EQUAL_STRING("value1", params[0].value);
    TEST_ASSERT_EQUAL_STRING("key2", params[1].key);
    TEST_ASSERT_EQUAL_STRING("value2", params[1].value);
    TEST_ASSERT_EQUAL_STRING("key3", params[2].key);
    TEST_ASSERT_EQUAL_STRING("value3", params[2].value);
    
    // Test empty value
    result = parse_query_string("key=", 4, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING("key", params[0].key);
    TEST_ASSERT_EQUAL_STRING("", params[0].value);
    
    // Test URL encoded values
    result = parse_query_string("search=hello%20world&filter=test%2Bvalue", 40, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("search", params[0].key);
    TEST_ASSERT_EQUAL_STRING("hello world", params[0].value);
    TEST_ASSERT_EQUAL_STRING("filter", params[1].key);
    TEST_ASSERT_EQUAL_STRING("test+value", params[1].value);
    
    // Test plus as space
    result = parse_query_string("name=John+Doe&city=New+York", 28, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("John Doe", params[0].value);
    TEST_ASSERT_EQUAL_STRING("New York", params[1].value);
    
    // Test no equals
    result = parse_query_string("key_without_value", 17, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING("key_without_value", params[0].key);
    TEST_ASSERT_EQUAL_STRING("", params[0].value);
    
    // Test empty string
    result = parse_query_string("", 0, params, MAX_QUERY_PARAMS);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Test max params exceeded
    result = parse_query_string("a=1&b=2&c=3", 12, params, 2);
    TEST_ASSERT_EQUAL_INT(2, result);
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
    RUN_TEST(test_function_url_decode);
    RUN_TEST(test_function_path_normalize);
    RUN_TEST(test_function_parse_query_string);
    return UNITY_END();
}