#ifndef URL_DECODE_H
#define URL_DECODE_H

#include "url.h"
#include <string.h>

/*
 * Compatibility wrapper for older call sites that use:
 *   url_decode(src, dst, dst_size)
 *
 * The underlying implementation is:
 *   int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len)
 *
 * Note: (url_decode)(...) avoids recursive macro expansion.
 */
#define url_decode(src, dst, dst_size) \
    (url_decode)((dst), (dst_size), (src), strlen((src)))

#endif /* URL_DECODE_H */

