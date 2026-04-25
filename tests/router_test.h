#ifndef ROUTER_TEST_H
#define ROUTER_TEST_H
#include <string.h>
#include "fixtures/requests.h"
#include "../parser/http_parser.h"
#include "../router/router.h"
#include "../router/handlers.h"
#include "unity/unity.h"


/* router/router.h */
//void router_dispatch(const HttpRequest *req, char *out, int out_size);
void test_function_router_dispatch(void);


/* router/handlers.h */
// void handle_index(const HttpRequest *req, char *out, int out_size);
void test_function_handle_index(void);
// void handle_echo(const HttpRequest *req, char *out, int out_size);
void test_function_handle_echo(void);
// void handle_headers(const HttpRequest *req, char *out, int out_size);
void test_function_handle_headers(void);
// void handle_health(const HttpRequest *req, char *out, int out_size);
void test_function_handle_health(void);
// void handle_post_echo(const HttpRequest *req, char *out, int out_size);
void test_function_handle_post_echo(void);

// void handle_not_found(const HttpRequest *req, char *out, int out_size);
void test_function_handle_not_found(void);
// void handle_method_not_allowed(const HttpRequest *req, char *out, int out_size);
void test_function_handle_method_not_allowed(void);
// void handle_bad_request(const HttpRequest *req, char *out, int out_size);
void test_function_handle_bad_request(void);
// void handle_internal_error(const HttpRequest *req, char *out, int out_size);
void test_function_handle_internal_error(void);



#endif /* ROUTER_TEST_H */