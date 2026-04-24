# Architecture / File Interaction Map

This project is a small single-threaded HTTP server with these layers:

- **Socket layer**: accepts TCP connections, reads/writes bytes
- **Parser layer**: turns raw bytes into an `HttpRequest`
- **Router layer**: maps `HttpRequest` → handler function
- **Response layer**: builds serialized HTTP responses

### End-to-end request flow

```text
main.c
  -> server_socket_create()                     (socket/connection.c)
  -> server_accept_loop()                       (socket/connection.c)
      -> accept()
      -> connection_handle()                    (socket/connection.c)
          -> recv_request()                     (socket/connection.c, static)
          -> http_parse_request()               (parser/http_parser.c)
              -> parse_request_line()           (parser/http_parser.c, static)
                  -> url_decode()               (parser/url.c)
                  -> path_normalize()           (parser/url.c)
                  -> parse_query_string()       (parser/url.c)
              -> parse_headers()                (parser/http_parser.c, static)
          -> router_dispatch()                  (router/router.c)
              -> handle_*()                     (router/handlers.c)
                  -> response_build_*()         (response/response_builder.c)
          -> send_all()                         (socket/connection.c, static)
          -> http_request_free()                (parser/http_parser.c)
```

### Modules and dependencies (by folder)

#### `main.c`

- **Depends on**: `socket/connection.h`
- **Role**: chooses port, creates listening socket, enters accept loop

#### `socket/`

##### `socket/connection.c` + `socket/connection.h`

- **Depends on**:
  - `parser/http_parser.h` (+ `parser/request.h`) for `HttpRequest` parsing/freeing
  - `router/router.h` (+ `router/handlers.h`) to dispatch parsed requests
  - `response/response_builder.h` for buffer sizing (`RESPONSE_TOTAL_MAX`)
  - OS socket headers (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`, `unistd.h`)
- **Exports**:
  - `server_socket_create(port, backlog)`
  - `server_accept_loop(server_fd)`
  - `connection_handle(client_fd, client_addr)`
- **Role**:
  - reads bytes from client
  - parses request
  - routes to handler
  - writes serialized response back to client

#### `parser/`

##### `parser/request.h`

- **Defines**:
  - `HttpRequest` (method/path/version/headers/query/body + convenience fields)
  - `ParseResult` error codes

##### `parser/http_parser.c` + `parser/http_parser.h`

- **Depends on**: `parser/request.h`, `parser/url.h`
- **Exports**:
  - `http_parse_request(buf, buf_len, &req)`
  - `http_request_free(&req)`
  - `http_get_header(&req, name)`
  - `http_parse_error_status(result)`
- **Role**: parses HTTP/1.0/1.1 request line, headers, and optional body

##### `parser/url.c` + `parser/url.h`

- **Depends on**: `parser/request.h` for constants/types used by helpers
- **Exports**:
  - `url_decode(dst, dst_size, src, src_len)`
  - `path_normalize(path)`
  - `parse_query_string(qs, len, params, max_params)`
- **Role**: URL percent-decoding, path normalization (anti-traversal), query parsing

##### `parser/url_decode.h`

- **Role**: compatibility wrapper header. Not required by the current code paths if
  callers include `parser/url.h` directly.

#### `router/`

##### `router/router.c` + `router/router.h`

- **Depends on**:
  - `parser/request.h` for `HttpRequest`
  - `router/handlers.h` for `handle_*` symbols
- **Exports**: `router_dispatch(req, out, out_size)`
- **Role**: matches `(req->method, req->path)` against a static route table

##### `router/handlers.c` + `router/handlers.h`

- **Depends on**:
  - `response/response_builder.h` to construct responses
  - `parser/url.h` for `url_decode()` (used by `/echo` query string)
- **Exports**: `handle_index`, `handle_echo`, `handle_headers`, `handle_health`,
  `handle_post_echo`, plus generic error handlers
- **Role**: implements endpoint behavior and populates response buffer

#### `response/`

##### `response/response.h`

- **Defines**: `HttpResponse` and header/body storage limits

##### `response/response_builder.c` + `response/response_builder.h`

- **Depends on**: `response/response.h`
- **Exports**:
  - `resp_init`, `resp_free`
  - `resp_add_header`, `resp_set_content_type`
  - `resp_set_body_str`, `resp_set_body_bytes`
  - `resp_serialize`
  - `resp_make_error`, `resp_make_json`, `resp_make_redirect`, `resp_make_no_content`
- **Role**: builds and serializes HTTP responses

### Key data contracts

- **Request object**: `HttpRequest` (`parser/request.h`) is the common request type
  shared across socket → router → handlers.
- **Response bytes**: handlers write into an output buffer (allocated in
  `socket/connection.c`) using response-builder helper functions.

