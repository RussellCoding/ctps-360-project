#ifndef REQUEST_H
#define REQUEST_H

#include <stddef.h>

/* ── limits ──────────────────────────────────────────────────── */
#define MAX_METHOD       16
#define MAX_PATH       2048
#define MAX_VERSION      16
#define MAX_HEADERS      64
#define MAX_QUERY_PARAMS 64
#define MAX_HEADER_NAME 256
#define MAX_HEADER_VAL 4096
#define MAX_PARAM_KEY  256
#define MAX_PARAM_VAL  2048
#define MAX_BODY_SIZE  (1024 * 1024 * 10)  /* 10 MB */

/* ── sub-types ───────────────────────────────────────────────── */
typedef struct {
    char name[MAX_HEADER_NAME];
    char value[MAX_HEADER_VAL];
} HttpHeader;

typedef struct {
    char key[MAX_PARAM_KEY];
    char value[MAX_PARAM_VAL];
} QueryParam;

/* ── parse result codes ──────────────────────────────────────── */
typedef enum {
    PARSE_OK                  =  0,
    PARSE_ERR_INCOMPLETE      = -1,   /* need more data          */
    PARSE_ERR_BAD_REQUEST     = -2,   /* malformed syntax        */
    PARSE_ERR_METHOD_TOO_LONG = -3,
    PARSE_ERR_URI_TOO_LONG    = -4,
    PARSE_ERR_VERSION_UNKNOWN = -5,
    PARSE_ERR_HEADER_OVERFLOW = -6,
    PARSE_ERR_BODY_TOO_LARGE  = -7,
    PARSE_ERR_NO_HOST         = -8,   /* HTTP/1.1 requires Host  */
    PARSE_ERR_NO_CONTENT_LEN  = -9,   /* POST/PUT without length */
} ParseResult;

/* ── the main struct your teammates consume ──────────────────── */
typedef struct {
    char        method[MAX_METHOD];
    char        path[MAX_PATH];        /* decoded, no query string */
    char        raw_path[MAX_PATH];    /* exactly as received      */
    char        version[MAX_VERSION];  /* "1.0" or "1.1"           */

    HttpHeader  headers[MAX_HEADERS];
    int         header_count;

    QueryParam  query_params[MAX_QUERY_PARAMS];
    int         query_param_count;

    char       *body;                  /* heap-allocated, may be NULL */
    size_t      body_length;

    /* convenience fields extracted from headers */
    size_t      content_length;        /* 0 if not present         */
    int         keep_alive;            /* 1 = keep-alive, 0 = close */
    int         chunked;               /* 1 = Transfer-Encoding: chunked */
} HttpRequest;

#endif /* REQUEST_H */