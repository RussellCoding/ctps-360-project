#include "url.h"
#include "request.h"

#include <string.h>
#include <ctype.h>
#include <stdio.h>

/* ── helpers ─────────────────────────────────────────────────── */

static int hex_val(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* ── url_decode ──────────────────────────────────────────────── */

int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len) {
    size_t di = 0;
    size_t si = 0;

    while (si < src_len) {
        if (di + 1 >= dst_size)
            return -1;  /* dst too small */

        if (src[si] == '%') {
            if (si + 2 >= src_len)
                return -1;  /* truncated escape */
            int hi = hex_val(src[si + 1]);
            int lo = hex_val(src[si + 2]);
            if (hi < 0 || lo < 0)
                return -1;  /* invalid hex digits */
            dst[di++] = (char)((hi << 4) | lo);
            si += 3;
        } else if (src[si] == '+') {
            /* application/x-www-form-urlencoded: + means space */
            dst[di++] = ' ';
            si++;
        } else {
            dst[di++] = src[si++];
        }
    }

    dst[di] = '\0';
    return 0;
}

/* ── path_normalize ──────────────────────────────────────────── */

int path_normalize(char *path) {
    /* Work through the path segment by segment using a small stack of
     * segment start offsets so we can back up on "..". */
    if (!path || path[0] != '/')
        return -1;

    char out[MAX_PATH];
    size_t oi = 0;
    const char *p = path;

    /* segments[i] is the index into out[] where segment i begins */
    int seg_starts[256];
    int depth = 0;

    while (*p) {
        if (*p == '/') {
            /* start of a new segment */
            p++;
            const char *seg = p;
            while (*p && *p != '/') p++;
            size_t seg_len = (size_t)(p - seg);

            if (seg_len == 0 || (seg_len == 1 && seg[0] == '.')) {
                /* empty or "." — skip */
                continue;
            }

            if (seg_len == 2 && seg[0] == '.' && seg[1] == '.') {
                /* ".." — pop one level */
                if (depth > 0) {
                    oi = seg_starts[--depth];
                } else {
                    /* tried to escape root */
                    return -1;
                }
                continue;
            }

            /* normal segment — record where it starts and append */
            if (oi + seg_len + 2 >= MAX_PATH)
                return -1;

            seg_starts[depth++] = (int)oi;
            if (depth >= 256) return -1;

            out[oi++] = '/';
            memcpy(out + oi, seg, seg_len);
            oi += seg_len;
        } else {
            p++;
        }
    }

    if (oi == 0) {
        out[oi++] = '/';
    }
    out[oi] = '\0';
    memcpy(path, out, oi + 1);
    return 0;
}

/* ── parse_query_string ──────────────────────────────────────── */

int parse_query_string(const char *qs, size_t len,
                       QueryParam *params, int max_params) {
    if (!qs || len == 0) return 0;

    int count = 0;
    const char *p = qs;
    const char *end = qs + len;

    while (p < end && count < max_params) {
        /* find '=' */
        const char *eq = p;
        while (eq < end && *eq != '=' && *eq != '&') eq++;

        if (eq >= end || *eq == '&') {
            /* key with no value — store with empty value */
            if (url_decode(params[count].key, MAX_PARAM_KEY, p, (size_t)(eq - p)) < 0)
                return -1;
            params[count].value[0] = '\0';
            count++;
            p = (eq < end) ? eq + 1 : end;
            continue;
        }

        /* decode key */
        if (url_decode(params[count].key, MAX_PARAM_KEY, p, (size_t)(eq - p)) < 0)
            return -1;

        /* find end of value */
        const char *val_start = eq + 1;
        const char *amp = val_start;
        while (amp < end && *amp != '&') amp++;

        /* decode value */
        if (url_decode(params[count].value, MAX_PARAM_VAL,
                       val_start, (size_t)(amp - val_start)) < 0)
            return -1;

        count++;
        p = (amp < end) ? amp + 1 : end;
    }

    return count;
}