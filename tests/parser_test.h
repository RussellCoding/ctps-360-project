#ifndef PARSER_TEST_H
#define PARSER_TEST_H
#include "./unity/unity.h"
#include "../parser/http_parser.h"
#include "../parser/url.h"
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

//int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len);
void test_function_url_decode(void);

//int path_normalize(char *path);
void test_function_path_normalize(void);

// int parse_query_string(const char *qs, size_t len, QueryParam *params, int max_params);
void test_function_parse_query_string(void);

#endif /* PARSER_TEST_H */