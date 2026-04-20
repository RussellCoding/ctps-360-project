#ifndef URL_H
#define URL_H

#include <stddef.h>

/*
 * Percent-decode a URL-encoded string in-place.
 * e.g. "hello%20world" -> "hello world"
 * Returns 0 on success, -1 on malformed encoding.
 */
int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len);

/*
 * Normalize a URL path in-place:
 *   - Resolve /./  -> /
 *   - Resolve /../ -> parent
 *   - Collapse multiple slashes
 * Returns 0 on success, -1 if path tries to escape root (path traversal).
 */
int path_normalize(char *path);

/*
 * Parse a query string "key=val&key2=val2" into parallel key/value arrays.
 * Returns number of params parsed, or -1 on error.
 */
#include "request.h"
int parse_query_string(const char *qs, size_t len,
                       QueryParam *params, int max_params);

#endif /* URL_H */