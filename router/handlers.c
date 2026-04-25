#include "handlers.h"
#include "../response/response_builder.h"
#include "../parser/url.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


/*
    static html welcome page to the root
*/
void handle_index(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_init(&resp, 200);
    
    const char *body =
        "<!DOCTYPE html>\n"
        "<html><head><title>Hello</title></head>\n"
        "<body><h1>Welcome to MyHTTPServer</h1>"
        "<p>The server is running.</p></body></html>\n";
    
    resp_set_body_str(&resp, body, "text/html; charset=utf-8");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}


/*
    gets query string form the url, decodes url encoded char
    sends the text back to the client
*/
void handle_echo(const HttpRequest *req, char *out, int out_size) {
    char body[1024];
    const char *qs = strchr(req->raw_path, '?');
    if (qs && qs[1] != '\0') {
        /* decode query string (without the '?') */
        char decoded[1024];
        if (url_decode(decoded, sizeof(decoded), qs + 1, strlen(qs + 1)) < 0) {
            snprintf(body, sizeof(body), "(malformed query string)\n");
        } else {
            snprintf(body, sizeof(body), "Query: %s\n", decoded);
        }
    } else {
        snprintf(body, sizeof(body), "(no query string)\n");
    }
    
    HttpResponse resp;
    resp_init(&resp, 200);
    resp_set_body_str(&resp, body, "text/plain");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}


/*
    returns all htp headers and reflects them to the client as plain text
*/
void handle_headers(const HttpRequest *req, char *out, int out_size) {
    char body[4096];
    int  pos = 0;
    //path to the start of the body and append the request method
    pos += snprintf(body + pos, sizeof(body) - pos,
                    "Method: %s\nPath: %s\n\n",
                    req->method, req->path);
    // loops through header array up to limit
    for (int i = 0; i < req->header_count && pos < (int)sizeof(body) - 128; i++) {
        pos += snprintf(body + pos, sizeof(body) - pos,
                        "%s: %s\n",
                        req->headers[i].name,
                        req->headers[i].value);
    }
    
    HttpResponse resp;
    resp_init(&resp, 200);
    resp_set_body_str(&resp, body, "text/plain");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}


/*
    provides json status to verify
*/
void handle_health(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_make_json(&resp, 200, "{\"status\":\"ok\"}");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}


/*
    takes raw payload from a post request and will return it exactly
    as it was received
*/
void handle_post_echo(const HttpRequest *req, char *out, int out_size) {
    HttpResponse resp;
    resp_init(&resp, 200);
    
    if (req->body && req->body_length > 0) {
        resp_set_body_bytes(&resp, req->body, req->body_length, "text/plain");
    } else {
        resp_set_body_str(&resp, "(empty body)\n", "text/plain");
    }
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}


//from here on out theese are generic error handlers 
/* 404 not found
    when reuqested uri doesnt match any registered route
 */
void handle_not_found(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_make_error(&resp, 404, "Not Found");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}

/* 405 method not allowed
    when route exists but http method is incorrect 
 */
void handle_method_not_allowed(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_make_error(&resp, 405, "Method Not Allowed");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}

/* 400 bad request
    request format is malformed for violate protocol
 */
void handle_bad_request(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_make_error(&resp, 400, "Bad Request");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}

/* 500 internal server error
    all server-side failures
 */
void handle_internal_error(const HttpRequest *req, char *out, int out_size) {
    (void)req;
    HttpResponse resp;
    resp_make_error(&resp, 500, "Internal Server Error");
    
    char *serialized;
    size_t serialized_len;
    resp_serialize(&resp, &serialized, &serialized_len);
    
    if (serialized_len < (size_t)out_size) {
        memcpy(out, serialized, serialized_len);
    }
    
    free(serialized);
    resp_free(&resp);
}