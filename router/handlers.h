#ifndef HANDLERS_H
#define HANDLERS_H

#include "../parser/request.h"

void handle_index(const HttpRequest *req, char *out, int out_size);
void handle_echo(const HttpRequest *req, char *out, int out_size);
void handle_headers(const HttpRequest *req, char *out, int out_size);
void handle_health(const HttpRequest *req, char *out, int out_size);
void handle_post_echo(const HttpRequest *req, char *out, int out_size);

void handle_not_found(const HttpRequest *req, char *out, int out_size);
void handle_method_not_allowed(const HttpRequest *req, char *out, int out_size);
void handle_bad_request(const HttpRequest *req, char *out, int out_size);
void handle_internal_error(const HttpRequest *req, char *out, int out_size);

#endif /* HANDLERS_H */

