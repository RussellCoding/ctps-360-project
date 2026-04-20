#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include "request.h"

/*
 * Parse a complete HTTP/1.x request from a raw byte buffer.
 *
 * buf      - raw bytes received from the socket
 * buf_len  - number of valid bytes in buf
 * req      - output struct; caller must call http_request_free() when done
 *
 * Returns:
 *   PARSE_OK            - success, req is fully populated
 *   PARSE_ERR_INCOMPLETE - not enough data yet; keep reading and call again
 *   PARSE_ERR_*          - hard error; respond with appropriate HTTP error
 *
 * Example:
 *   HttpRequest req;
 *   ParseResult r = http_parse_request(buf, len, &req);
 *   if (r == PARSE_OK) {
 *       // hand req to your router
 *       http_request_free(&req);
 *   }
 */
ParseResult http_parse_request(const char *buf, size_t buf_len,
                               HttpRequest *req);

/*
 * Free any heap memory owned by req (i.e. req->body).
 * Safe to call on a zero-initialised req.
 */
void http_request_free(HttpRequest *req);

/*
 * Look up a header value by name (case-insensitive).
 * Returns pointer into req->headers[i].value, or NULL if not found.
 */
const char *http_get_header(const HttpRequest *req, const char *name);

/*
 * Map a ParseResult error code to a recommended HTTP status code string,
 * e.g. PARSE_ERR_BAD_REQUEST -> "400 Bad Request"
 */
const char *http_parse_error_status(ParseResult result);

#endif /* HTTP_PARSER_H */