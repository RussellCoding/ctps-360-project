#ifndef RESPONSE_TEST_H
#define RESPONSE_TEST_H
#include <stdlib.h>
#include <string.h>
#include "./unity/unity.h"
#include "../response/response_builder.h"
#include "../response/response.h"

/* Initialise a response with a status code. Zero-fills everything else. */
void test_function_resp_init(void);

/* Free heap memory owned by resp (body if body_owned, nothing else). */
void test_function_resp_free(void);

/* ── headers ─────────────────────────────────────────────────── */

/*
 * Add or overwrite a header (case-insensitive name match).
 * Returns 0 on success, -1 if header table is full or args are too long.
 */
void test_function_resp_add_header(void);

/* Convenience: set Content-Type header. */
void test_function_resp_set_content_type(void);

/* ── body ────────────────────────────────────────────────────── */

/*
 * Set body from a C string (makes an internal copy).
 * Also sets Content-Length and Content-Type automatically.
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_set_body_str(void);

/*
 * Set body from raw bytes (makes an internal copy).
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_set_body_bytes(void);

/* ── serialization ───────────────────────────────────────────── */

/*
 * Serialize the response into a heap-allocated byte buffer ready to
 * write to a socket.
 *
 * On success: *out points to the buffer, *out_len is its length.
 *             Caller must free(*out).
 * Returns 0 on success, -1 on allocation failure.
 */
void test_function_resp_serialize(void);

/* ── convenience constructors ────────────────────────────────── */

/* Build a plain-text error response (e.g. 404, 400) in one call. */
void test_function_resp_make_error(void);

/* Build a JSON response in one call. */
void test_function_resp_make_json(void);

/* Build a redirect response (301 or 302). */
void test_function_resp_make_redirect(void);

/* Build a 204 No Content response. */
void test_function_resp_make_no_content(void);

/* ── status helpers ──────────────────────────────────────────── */

/* Return the standard reason phrase for a status code.
 * e.g. 404 -> "Not Found". Returns "Unknown" for unrecognised codes. */
void test_function_resp_status_text(void);

#endif /* RESPONSE_TEST_H */