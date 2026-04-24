#include "unity.h"
#include "../parser/http_parser.h"
#include "fixtures/requests.h"
#include "string.h"

// ParseResult http_parse_request(const char *buf, size_t buf_len, HttpRequest *req);
void test_function_http_parse_request(void);

// void http_request_free(HttpRequest *req);
void test_function_http_request_free(void);

// const char *http_get_header(const HttpRequest *req, const char *name);
void test_function_http_get_header(void);

// const char *http_parse_error_status(ParseResult result);
void test_function_http_parse_error_status(void);
