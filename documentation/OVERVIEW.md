# File Guide (what each `.c` file does)

This document has **one section per C source file** describing what it does, how it works internally, and what it depends on.

## `main.c`

**Purpose**
- Program entry point. Picks a listening port and starts the server.

**Key functions**
- `main(argc, argv)`

**How it works**
- Sets a default port (`DEFAULT_PORT`, 8080).
- If a single CLI arg is provided, parses it with `atoi()` and validates it’s in \(1..65535\).
- Calls `server_socket_create(port, BACKLOG)` to create/bind/listen.
- Calls `server_accept_loop(server_fd)` which blocks forever accepting clients.

**Dependencies / interactions**
- Includes `socket/connection.h`.
- Hands control to the socket layer; does not deal with HTTP parsing or routing itself.

---

## `socket/connection.c`

**Purpose**
- Owns the **TCP server lifecycle**: create the listening socket, accept clients, read raw bytes, send responses.
- Orchestrates the end-to-end HTTP request flow by calling into the parser and router.

**Key exported functions**
- `int server_socket_create(int port, int backlog)`
- `void server_accept_loop(int server_fd)`
- `void connection_handle(int client_fd, struct sockaddr_in *client_addr)`

**Important internal helpers**
- `static int recv_request(int fd, char *buf, int buf_size)`
  - Reads from the socket until it sees an end-of-headers marker (`\r\n\r\n` or `\n\n`), or the buffer fills.
- `static int send_all(int fd, const char *buf, int len)`
  - Loops until all bytes are written via `send()`.

**How it works (control flow)**
- `server_socket_create()`
  - `socket()` → `setsockopt(SO_REUSEADDR)` → `bind()` → `listen()`.
  - Returns the listening `fd` or `-1` on error.
- `server_accept_loop()`
  - Calls `accept()` in a loop.
  - For each accepted client, calls `connection_handle(client_fd, &client_addr)`.
- `connection_handle()`
  - Allocates `read_buf` and `write_buf`.
  - Loop for keep-alive:
    - `recv_request()` into `read_buf`.
    - `http_parse_request(read_buf, nread, &req)` to build an `HttpRequest`.
    - On parse success: `router_dispatch(&req, write_buf, WRITE_BUF_SIZE)`.
    - On parse failure: builds a 400 error response.
    - Sends response bytes with `send_all()`.
    - Frees any request-owned heap memory via `http_request_free(&req)`.
    - Breaks if `req.keep_alive` is false.

**Dependencies / interactions**
- Parser: `parser/http_parser.h` + `parser/request.h` (uses `HttpRequest`, `ParseResult`).
- Router: `router/router.h` (dispatch) and `router/handlers.h` (symbols referenced by router table).
- Response: `response/response_builder.h` (for constants and helper builders).
- OS: POSIX socket headers and `unistd.h` (`close()`).

---

## `parser/http_parser.c`

**Purpose**
- Parses a complete HTTP/1.0 or HTTP/1.1 request from a byte buffer into a structured `HttpRequest`.

**Key exported functions**
- `ParseResult http_parse_request(const char *buf, size_t buf_len, HttpRequest *req)`
- `void http_request_free(HttpRequest *req)`
- `const char *http_get_header(const HttpRequest *req, const char *name)`
- `const char *http_parse_error_status(ParseResult result)`

**How it works (major phases)**
- **Request line parsing** (`parse_request_line`, static)
  - Splits `METHOD SP URI SP HTTP/VERSION`.
  - Validates method is uppercase token and within `MAX_METHOD`.
  - Copies raw URI into `req->raw_path`.
  - Extracts `req->version` as `"1.0"` or `"1.1"`.
  - Splits path vs query at `?`:
    - `req->path` holds the path portion
    - `parse_query_string(...)` fills `req->query_params[]`
  - Percent-decodes the path using `url_decode(...)`.
  - Normalizes the path using `path_normalize(...)` (guards path traversal).
- **Header parsing** (`parse_headers`, static)
  - Scans lines until a blank line, populates `req->headers[]`.
  - Enforces `MAX_HEADERS`, `MAX_HEADER_NAME`, `MAX_HEADER_VAL`.
- **Convenience fields** (`extract_convenience_headers`, static)
  - Reads `Content-Length`, `Connection`, `Transfer-Encoding`.
  - Sets `req->content_length`, `req->keep_alive`, `req->chunked`.
- **Body parsing**
  - If `Transfer-Encoding: chunked`, reads and concatenates chunks into a single heap buffer.
  - Else if `Content-Length > 0`, copies that many bytes into a heap buffer.
  - Tracks body size against `MAX_BODY_SIZE`.

**Memory ownership**
- `req->body` is heap-allocated by the parser when there is a body.
- Callers must call `http_request_free(&req)` to avoid leaks.

**Dependencies / interactions**
- Uses `parser/url.h` for `url_decode`, `path_normalize`, `parse_query_string`.
- Defines a strict contract for router/handlers: they consume `HttpRequest` fields.

---

## `parser/url.c`

**Purpose**
- Provides URL utility functions used by the parser and handlers:
  - percent-decoding
  - path normalization (anti-traversal)
  - query string parsing into key/value pairs

**Key exported functions**
- `int url_decode(char *dst, size_t dst_size, const char *src, size_t src_len)`
- `int path_normalize(char *path)`
- `int parse_query_string(const char *qs, size_t len, QueryParam *params, int max_params)`

**How it works**
- `url_decode()`
  - Walks `src` and writes into `dst`.
  - `%HH` sequences become a single byte.
  - `+` becomes space (form encoding convention).
  - Returns `-1` on invalid encoding or if `dst` is too small.
- `path_normalize()`
  - Requires paths begin with `/`.
  - Collapses redundant slashes, removes `.` segments.
  - Processes `..` segments by popping a segment stack; returns `-1` if it tries to escape root.
  - Writes the normalized path back into the input buffer.
- `parse_query_string()`
  - Parses `key=value&k2=v2` pairs up to `max_params`.
  - Decodes both keys and values using `url_decode()`.
  - Supports keys without values (`key&...`) as empty-string values.

**Dependencies / interactions**
- Used by `parser/http_parser.c` and `router/handlers.c` (for `/echo`).

---

## `router/router.c`

**Purpose**
- Implements URL routing: maps `(method, path)` to a handler function.

**Key exported function**
- `void router_dispatch(const HttpRequest *req, char *out, int out_size)`

**How it works**
- Defines a function pointer type `HandlerFn(const HttpRequest*, char*, int)`.
- Defines a static `route_table[]` with entries like `{ "GET", "/echo", handle_echo }`.
- Dispatch logic:
  - Iterates routes; compares `req->path` against route `.path`.
  - If a path matches and method matches (or route method is `NULL`), calls the handler and returns.
  - If at least one path matched but no method matched, calls `handle_method_not_allowed()`.
  - Otherwise calls `handle_not_found()`.

**Dependencies / interactions**
- Uses handler functions from `router/handlers.c`.
- Treats `req->path` as the canonical (decoded, normalized, no query string) route key.

---

## `router/handlers.c`

**Purpose**
- Implements endpoint behavior (the “controllers”).
- Builds response bytes into a caller-provided output buffer by calling response builder helpers.

**Key exported functions**
- Route handlers:
  - `handle_index` (GET `/`)
  - `handle_echo` (GET `/echo`)
  - `handle_post_echo` (POST `/echo`)
  - `handle_headers` (GET `/headers`)
  - `handle_health` (GET `/health`)
- Generic error handlers:
  - `handle_not_found` (404)
  - `handle_method_not_allowed` (405)
  - `handle_bad_request` (400)
  - `handle_internal_error` (500)

**How it works (high level)**
- Each handler inspects the `HttpRequest` and calls a `response_build_*` helper to serialize an HTTP response into `out`.
- `/echo` (GET)
  - Finds the query string by searching for `?` in `req->raw_path`.
  - Decodes the query text via `url_decode()` before reflecting it back.
- `/headers`
  - Iterates `req->headers[]` up to `req->header_count` and prints them into a plaintext body.
- `/echo` (POST)
  - If `req->body` exists and `req->body_length > 0`, reflects it back.

**Dependencies / interactions**
- Uses `response/response_builder.h` for response serialization helpers.
- Uses `parser/url.h` for query decoding in `/echo`.

---

## `response/response_builder.c`

**Purpose**
- Provides a structured way to build HTTP responses (`HttpResponse`) and serialize them to raw bytes.

**Key exported functions (core lifecycle)**
- `resp_init(resp, status_code)`
- `resp_add_header(resp, name, value)`
- `resp_set_body_str(resp, body, mime)` / `resp_set_body_bytes(...)`
- `resp_serialize(resp, &out, &out_len)`
- `resp_free(resp)`

**How it works**
- Maintains an in-struct header table (`RespHeader headers[MAX_RESP_HEADERS]`).
- `resp_add_header()` overwrites existing headers case-insensitively.
- Body setters allocate and own a heap copy of the body (`resp->body_owned = 1`).
- `resp_serialize()`:
  - Estimates a buffer size.
  - Writes status line: `HTTP/1.1 <code> <reason>\r\n`
  - Writes headers, then blank line, then body bytes.

**Convenience constructors**
- `resp_make_error()`: builds a basic HTML error and forces `Connection: close`.
- `resp_make_json()`, `resp_make_redirect()`, `resp_make_no_content()`.

**Dependencies / interactions**
- Called by router handlers indirectly through `response_builder.h` helpers.
- Depends on `response/response.h` for the `HttpResponse` struct and limits.

