#include "http_parser.h"
#include "url.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/* ── internal helpers ────────────────────────────────────────── */

/* Case-insensitive strncmp */
static int strncasecmp_ascii(const char *a, const char *b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        int diff = tolower((unsigned char)a[i]) - tolower((unsigned char)b[i]);
        if (diff != 0) return diff;
        if (a[i] == '\0') return 0;
    }
    return 0;
}

/* Find \r\n in a buffer. Returns pointer to \r, or NULL. */
static const char *find_crlf(const char *buf, size_t len) {
    for (size_t i = 0; i + 1 < len; i++) {
        if (buf[i] == '\r' && buf[i + 1] == '\n')
            return buf + i;
    }
    return NULL;
}

/* ── request line ────────────────────────────────────────────── */

static ParseResult parse_request_line(const char *line, size_t len,
                                       HttpRequest *req) {
    /* FORMAT: METHOD SP Request-URI SP HTTP/Version */
    const char *p = line;
    const char *end = line + len;

    /* method */
    const char *sp = memchr(p, ' ', (size_t)(end - p));
    if (!sp) return PARSE_ERR_BAD_REQUEST;

    size_t method_len = (size_t)(sp - p);
    if (method_len >= MAX_METHOD)   return PARSE_ERR_METHOD_TOO_LONG;
    if (method_len == 0)            return PARSE_ERR_BAD_REQUEST;
    memcpy(req->method, p, method_len);
    req->method[method_len] = '\0';

    /* uppercase check — methods must be uppercase tokens */
    for (size_t i = 0; i < method_len; i++) {
        if (!isupper((unsigned char)req->method[i]) && req->method[i] != '_')
            return PARSE_ERR_BAD_REQUEST;
    }

    p = sp + 1;

    /* request-URI */
    sp = memchr(p, ' ', (size_t)(end - p));
    if (!sp) return PARSE_ERR_BAD_REQUEST;

    size_t uri_len = (size_t)(sp - p);
    if (uri_len == 0)         return PARSE_ERR_BAD_REQUEST;
    if (uri_len >= MAX_PATH)  return PARSE_ERR_URI_TOO_LONG;

    memcpy(req->raw_path, p, uri_len);
    req->raw_path[uri_len] = '\0';
    p = sp + 1;

    /* HTTP version */
    size_t ver_len = (size_t)(end - p);
    if (ver_len == 0 || ver_len >= MAX_VERSION) return PARSE_ERR_BAD_REQUEST;

    if (strncasecmp_ascii(p, "HTTP/1.1", 8) == 0) {
        memcpy(req->version, "1.1", 4);
    } else if (strncasecmp_ascii(p, "HTTP/1.0", 8) == 0) {
        memcpy(req->version, "1.0", 4);
    } else {
        return PARSE_ERR_VERSION_UNKNOWN;
    }

    /* ── split path and query string ─────────────────────────── */
    char *q = memchr(req->raw_path, '?', uri_len);
    if (q) {
        size_t path_len = (size_t)(q - req->raw_path);
        memcpy(req->path, req->raw_path, path_len);
        req->path[path_len] = '\0';

        /* parse query string */
        int n = parse_query_string(q + 1, uri_len - path_len - 1,
                                   req->query_params, MAX_QUERY_PARAMS);
        if (n < 0) return PARSE_ERR_BAD_REQUEST;
        req->query_param_count = n;
    } else {
        memcpy(req->path, req->raw_path, uri_len + 1);
        req->query_param_count = 0;
    }

    /* percent-decode the path */
    char decoded[MAX_PATH];
    if (url_decode(decoded, MAX_PATH, req->path, strlen(req->path)) < 0)
        return PARSE_ERR_BAD_REQUEST;
    memcpy(req->path, decoded, strlen(decoded) + 1);

    /* normalize path — guards against path traversal */
    if (path_normalize(req->path) < 0)
        return PARSE_ERR_BAD_REQUEST;

    return PARSE_OK;
}

/* ── headers ─────────────────────────────────────────────────── */

static ParseResult parse_headers(const char *buf, size_t len,
                                  HttpRequest *req,
                                  size_t *headers_end_offset) {
    const char *p = buf;
    const char *end = buf + len;

    while (p < end) {
        const char *crlf = find_crlf(p, (size_t)(end - p));
        if (!crlf) return PARSE_ERR_INCOMPLETE;

        size_t line_len = (size_t)(crlf - p);

        /* blank line = end of headers */
        if (line_len == 0) {
            *headers_end_offset = (size_t)(crlf + 2 - buf);
            return PARSE_OK;
        }

        if (req->header_count >= MAX_HEADERS)
            return PARSE_ERR_HEADER_OVERFLOW;

        /* find ':' */
        const char *colon = memchr(p, ':', line_len);
        if (!colon) return PARSE_ERR_BAD_REQUEST;

        size_t name_len = (size_t)(colon - p);
        if (name_len == 0 || name_len >= MAX_HEADER_NAME)
            return PARSE_ERR_BAD_REQUEST;

        /* header names must not contain whitespace */
        for (size_t i = 0; i < name_len; i++) {
            if (isspace((unsigned char)p[i]))
                return PARSE_ERR_BAD_REQUEST;
        }

        HttpHeader *h = &req->headers[req->header_count];

        memcpy(h->name, p, name_len);
        h->name[name_len] = '\0';

        /* skip OWS (optional whitespace) after colon */
        const char *val = colon + 1;
        while (val < crlf && (*val == ' ' || *val == '\t')) val++;

        size_t val_len = (size_t)(crlf - val);
        /* trim trailing OWS */
        while (val_len > 0 && (val[val_len-1] == ' ' || val[val_len-1] == '\t'))
            val_len--;

        if (val_len >= MAX_HEADER_VAL) return PARSE_ERR_BAD_REQUEST;

        memcpy(h->value, val, val_len);
        h->value[val_len] = '\0';

        req->header_count++;
        p = crlf + 2;
    }

    return PARSE_ERR_INCOMPLETE;
}

/* ── convenience header extraction ──────────────────────────── */

static void extract_convenience_headers(HttpRequest *req) {
    const char *cl = http_get_header(req, "Content-Length");
    if (cl) {
        req->content_length = (size_t)strtoul(cl, NULL, 10);
    }

    const char *conn = http_get_header(req, "Connection");
    if (conn) {
        req->keep_alive = (strncasecmp_ascii(conn, "keep-alive", 10) == 0) ? 1 : 0;
    } else {
        /* HTTP/1.1 defaults to keep-alive, 1.0 to close */
        req->keep_alive = (req->version[2] == '1') ? 1 : 0;
    }

    const char *te = http_get_header(req, "Transfer-Encoding");
    if (te && strncasecmp_ascii(te, "chunked", 7) == 0) {
        req->chunked = 1;
    }
}

/* ── chunked body reader ─────────────────────────────────────── */

static ParseResult read_chunked_body(const char *buf, size_t len,
                                      HttpRequest *req) {
    /* Accumulate chunks into a single allocation. */
    size_t capacity = 4096;
    char *body = malloc(capacity);
    if (!body) return PARSE_ERR_BAD_REQUEST;

    size_t body_len = 0;
    const char *p = buf;
    const char *end = buf + len;

    while (p < end) {
        /* read chunk size line (hex) */
        const char *crlf = find_crlf(p, (size_t)(end - p));
        if (!crlf) { free(body); return PARSE_ERR_INCOMPLETE; }

        size_t chunk_size = (size_t)strtoul(p, NULL, 16);
        p = crlf + 2;

        if (chunk_size == 0) break;  /* final chunk */

        if (body_len + chunk_size > MAX_BODY_SIZE) {
            free(body);
            return PARSE_ERR_BODY_TOO_LARGE;
        }

        if ((size_t)(end - p) < chunk_size + 2) {
            free(body);
            return PARSE_ERR_INCOMPLETE;
        }

        /* grow buffer if needed */
        while (body_len + chunk_size > capacity) {
            capacity *= 2;
            char *tmp = realloc(body, capacity);
            if (!tmp) { free(body); return PARSE_ERR_BAD_REQUEST; }
            body = tmp;
        }

        memcpy(body + body_len, p, chunk_size);
        body_len += chunk_size;
        p += chunk_size + 2;  /* skip trailing CRLF */
    }

    req->body = body;
    req->body_length = body_len;
    return PARSE_OK;
}

/* ── main entry point ────────────────────────────────────────── */

ParseResult http_parse_request(const char *buf, size_t buf_len,
                                HttpRequest *req) {
    memset(req, 0, sizeof(*req));

    /* ── 1. find end of request line ──────────────────────────── */
    const char *first_crlf = find_crlf(buf, buf_len);
    if (!first_crlf) return PARSE_ERR_INCOMPLETE;

    ParseResult r = parse_request_line(buf, (size_t)(first_crlf - buf), req);
    if (r != PARSE_OK) return r;

    /* ── 2. parse headers ─────────────────────────────────────── */
    const char *header_start = first_crlf + 2;
    size_t header_buf_len = buf_len - (size_t)(header_start - buf);
    size_t headers_end = 0;

    r = parse_headers(header_start, header_buf_len, req, &headers_end);
    if (r != PARSE_OK) return r;

    /* ── 3. extract convenience fields ───────────────────────── */
    extract_convenience_headers(req);

    /* ── 4. HTTP/1.1 must have Host header ───────────────────── */
    if (req->version[2] == '1' && !http_get_header(req, "Host"))
        return PARSE_ERR_NO_HOST;

    /* ── 5. body ──────────────────────────────────────────────── */
    const char *body_start = header_start + headers_end;
    size_t body_buf_len = buf_len - (size_t)(body_start - buf);

    int has_body = (strcmp(req->method, "POST")  == 0 ||
                    strcmp(req->method, "PUT")   == 0 ||
                    strcmp(req->method, "PATCH") == 0);

    if (req->chunked) {
        r = read_chunked_body(body_start, body_buf_len, req);
        if (r != PARSE_OK) return r;
    } else if (req->content_length > 0) {
        if (req->content_length > MAX_BODY_SIZE)
            return PARSE_ERR_BODY_TOO_LARGE;
        if (body_buf_len < req->content_length)
            return PARSE_ERR_INCOMPLETE;

        req->body = malloc(req->content_length + 1);
        if (!req->body) return PARSE_ERR_BAD_REQUEST;
        memcpy(req->body, body_start, req->content_length);
        req->body[req->content_length] = '\0';
        req->body_length = req->content_length;
    } else if (has_body) {
        return PARSE_ERR_NO_CONTENT_LEN;
    }

    return PARSE_OK;
}

/* ── helpers ─────────────────────────────────────────────────── */

void http_request_free(HttpRequest *req) {
    if (req && req->body) {
        free(req->body);
        req->body = NULL;
        req->body_length = 0;
    }
}

const char *http_get_header(const HttpRequest *req, const char *name) {
    size_t name_len = strlen(name);
    for (int i = 0; i < req->header_count; i++) {
        if (strncasecmp_ascii(req->headers[i].name, name, name_len) == 0 &&
            req->headers[i].name[name_len] == '\0') {
            return req->headers[i].value;
        }
    }
    return NULL;
}

const char *http_parse_error_status(ParseResult result) {
    switch (result) {
        case PARSE_ERR_BAD_REQUEST:     return "400 Bad Request";
        case PARSE_ERR_METHOD_TOO_LONG: return "400 Bad Request";
        case PARSE_ERR_URI_TOO_LONG:    return "414 URI Too Long";
        case PARSE_ERR_VERSION_UNKNOWN: return "505 HTTP Version Not Supported";
        case PARSE_ERR_HEADER_OVERFLOW: return "431 Request Header Fields Too Large";
        case PARSE_ERR_BODY_TOO_LARGE:  return "413 Content Too Large";
        case PARSE_ERR_NO_HOST:         return "400 Bad Request";
        case PARSE_ERR_NO_CONTENT_LEN:  return "411 Length Required";
        default:                        return "500 Internal Server Error";
    }
}