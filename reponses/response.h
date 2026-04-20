#ifndef RESPONSE_H
#define RESPONSE_H

#include <stddef.h>

/* ── limits ──────────────────────────────────────────────────── */
#define MAX_RESP_HEADERS     32
#define MAX_RESP_HEADER_NAME 256
#define MAX_RESP_HEADER_VAL  4096
#define MAX_STATUS_LEN       64

/* ── header pair ─────────────────────────────────────────────── */
typedef struct {
    char name[MAX_RESP_HEADER_NAME];
    char value[MAX_RESP_HEADER_VAL];
} RespHeader;

/* ── the response struct ─────────────────────────────────────── */
typedef struct {
    int        status_code;
    char       status_text[MAX_STATUS_LEN];  /* e.g. "OK", "Not Found" */

    RespHeader headers[MAX_RESP_HEADERS];
    int        header_count;

    char      *body;        /* heap-allocated, may be NULL */
    size_t     body_length;
    int        body_owned;  /* 1 = we free it, 0 = caller owns it */
} HttpResponse;

#endif /* RESPONSE_H */