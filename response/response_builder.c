#include "response_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── internal helpers ────────────────────────────────────────── */

static int strncasecmp_ascii(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int diff = tolower((unsigned char)a[i]) - tolower((unsigned char)b[i]);
        if (diff != 0) return diff;
        if (a[i] == '\0') return 0;
    }
    return 0;
}

/* ── status text table ───────────────────────────────────────── */

const char *resp_status_text(int code) {
    switch (code) {
        /* 1xx */
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        /* 2xx */
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 206: return "Partial Content";
        /* 3xx */
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        /* 4xx */
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 413: return "Content Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 422: return "Unprocessable Content";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        /* 5xx */
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown";
    }
}

/* ── lifecycle ───────────────────────────────────────────────── */

void resp_init(HttpResponse *resp, int status_code) {
    memset(resp, 0, sizeof(*resp));
    resp->status_code = status_code;
    const char *text = resp_status_text(status_code);
    strncpy(resp->status_text, text, MAX_STATUS_LEN - 1);
}

void resp_free(HttpResponse *resp) {
    if (resp && resp->body && resp->body_owned) {
        free(resp->body);
        resp->body = NULL;
        resp->body_length = 0;
        resp->body_owned = 0;
    }
}

/* ── headers ─────────────────────────────────────────────────── */

int resp_add_header(HttpResponse *resp, const char *name, const char *value) {
    if (!name || !value) return -1;
    if (strlen(name)  >= MAX_RESP_HEADER_NAME) return -1;
    if (strlen(value) >= MAX_RESP_HEADER_VAL)  return -1;

    /* overwrite if already present (case-insensitive) */
    size_t name_len = strlen(name);
    for (int i = 0; i < resp->header_count; i++) {
        if (strncasecmp_ascii(resp->headers[i].name, name, name_len) == 0 &&
            resp->headers[i].name[name_len] == '\0') {
            strncpy(resp->headers[i].value, value, MAX_RESP_HEADER_VAL - 1);
            return 0;
        }
    }

    if (resp->header_count >= MAX_RESP_HEADERS) return -1;

    RespHeader *h = &resp->headers[resp->header_count++];
    strncpy(h->name,  name,  MAX_RESP_HEADER_NAME - 1);
    strncpy(h->value, value, MAX_RESP_HEADER_VAL  - 1);
    return 0;
}

int resp_set_content_type(HttpResponse *resp, const char *mime) {
    return resp_add_header(resp, "Content-Type", mime);
}

/* ── body ────────────────────────────────────────────────────── */

int resp_set_body_bytes(HttpResponse *resp, const char *data, size_t len,
                        const char *mime) {
    resp_free(resp);  /* drop any existing body */

    char *copy = malloc(len + 1);
    if (!copy) return -1;
    memcpy(copy, data, len);
    copy[len] = '\0';

    resp->body        = copy;
    resp->body_length = len;
    resp->body_owned  = 1;

    /* Content-Length */
    char cl[32];
    snprintf(cl, sizeof(cl), "%zu", len);
    resp_add_header(resp, "Content-Length", cl);

    if (mime) resp_set_content_type(resp, mime);
    return 0;
}

int resp_set_body_str(HttpResponse *resp, const char *body, const char *mime) {
    return resp_set_body_bytes(resp, body, strlen(body), mime);
}

/* ── serialization ───────────────────────────────────────────── */

int resp_serialize(const HttpResponse *resp, char **out, size_t *out_len) {
    /* ── estimate buffer size ─────────────────────────────────── */
    /* status line: "HTTP/1.1 XXX Reason\r\n" */
    size_t cap = 64;
    /* headers */
    for (int i = 0; i < resp->header_count; i++) {
        cap += strlen(resp->headers[i].name) +
               strlen(resp->headers[i].value) + 4;  /* ": " + "\r\n" */
    }
    cap += 2;  /* blank line */
    cap += resp->body_length;
    cap += 64; /* padding */

    char *buf = malloc(cap);
    if (!buf) return -1;

    /* ── status line ──────────────────────────────────────────── */
    int written = snprintf(buf, cap, "HTTP/1.1 %d %s\r\n",
                           resp->status_code, resp->status_text);
    size_t pos = (size_t)written;

    /* ── headers ──────────────────────────────────────────────── */
    for (int i = 0; i < resp->header_count; i++) {
        written = snprintf(buf + pos, cap - pos, "%s: %s\r\n",
                           resp->headers[i].name, resp->headers[i].value);
        pos += (size_t)written;
    }

    /* ── blank line ───────────────────────────────────────────── */
    memcpy(buf + pos, "\r\n", 2);
    pos += 2;

    /* ── body ─────────────────────────────────────────────────── */
    if (resp->body && resp->body_length > 0) {
        memcpy(buf + pos, resp->body, resp->body_length);
        pos += resp->body_length;
    }

    *out     = buf;
    *out_len = pos;
    return 0;
}

/* ── convenience constructors ────────────────────────────────── */

void resp_make_error(HttpResponse *resp, int status_code,
                     const char *message) {
    resp_init(resp, status_code);

    /* simple HTML error page */
    char body[512];
    snprintf(body, sizeof(body),
             "<html><body><h1>%d %s</h1><p>%s</p></body></html>",
             status_code, resp_status_text(status_code),
             message ? message : "");
    resp_set_body_str(resp, body, "text/html; charset=utf-8");
    resp_add_header(resp, "Connection", "close");
}

void resp_make_json(HttpResponse *resp, int status_code,
                    const char *json_body) {
    resp_init(resp, status_code);
    if (json_body)
        resp_set_body_str(resp, json_body, "application/json");
}

void resp_make_redirect(HttpResponse *resp, int status_code,
                        const char *location) {
    resp_init(resp, status_code);
    resp_add_header(resp, "Location", location);
    resp_add_header(resp, "Content-Length", "0");
}

void resp_make_no_content(HttpResponse *resp) {
    resp_init(resp, 204);
    resp_add_header(resp, "Content-Length", "0");
}