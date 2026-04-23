#ifndef RESPONSE_BUILDER_H
#define RESPONSE_BUILDER_H

#include "response.h"
#include <stddef.h>

/*
 * ── lifecycle ────────────────────────────────────────────────────
 *
 * Typical usage:
 *
 *   HttpResponse resp;
 *   resp_init(&resp, 200);
 *   resp_set_body_str(&resp, "{\"ok\":true}", "application/json");
 *   resp_add_header(&resp, "X-Request-Id", "abc123");
 *
 *   char *raw;
 *   size_t raw_len;
 *   resp_serialize(&resp, &raw, &raw_len);
 *   // write raw/raw_len to socket
 *   free(raw);
 *   resp_free(&resp);
 */

/* Initialise a response with a status code. Zero-fills everything else. */
void resp_init(HttpResponse *resp, int status_code);

/* Free heap memory owned by resp (body if body_owned, nothing else). */
void resp_free(HttpResponse *resp);

/* ── headers ─────────────────────────────────────────────────── */

/*
 * Add or overwrite a header (case-insensitive name match).
 * Returns 0 on success, -1 if header table is full or args are too long.
 */
int resp_add_header(HttpResponse *resp, const char *name, const char *value);

/* Convenience: set Content-Type header. */
int resp_set_content_type(HttpResponse *resp, const char *mime);

/* ── body ────────────────────────────────────────────────────── */

/*
 * Set body from a C string (makes an internal copy).
 * Also sets Content-Length and Content-Type automatically.
 * Returns 0 on success, -1 on allocation failure.
 */
int resp_set_body_str(HttpResponse *resp, const char *body, const char *mime);

/*
 * Set body from raw bytes (makes an internal copy).
 * Returns 0 on success, -1 on allocation failure.
 */
int resp_set_body_bytes(HttpResponse *resp, const char *data, size_t len,
                        const char *mime);

/* ── serialization ───────────────────────────────────────────── */

/*
 * Serialize the response into a heap-allocated byte buffer ready to
 * write to a socket.
 *
 * On success: *out points to the buffer, *out_len is its length.
 *             Caller must free(*out).
 * Returns 0 on success, -1 on allocation failure.
 */
int resp_serialize(const HttpResponse *resp, char **out, size_t *out_len);

/* ── convenience constructors ────────────────────────────────── */

/* Build a plain-text error response (e.g. 404, 400) in one call. */
void resp_make_error(HttpResponse *resp, int status_code, const char *message);

/* Build a JSON response in one call. */
void resp_make_json(HttpResponse *resp, int status_code, const char *json_body);

/* Build a redirect response (301 or 302). */
void resp_make_redirect(HttpResponse *resp, int status_code,
                        const char *location);

/* Build a 204 No Content response. */
void resp_make_no_content(HttpResponse *resp);

/* ── status helpers ──────────────────────────────────────────── */

/* Return the standard reason phrase for a status code.
 * e.g. 404 -> "Not Found". Returns "Unknown" for unrecognised codes. */
const char *resp_status_text(int code);

#endif /* RESPONSE_BUILDER_H */