# ctps-360-project

A small **single-threaded HTTP server in C** (C11) built as a class project. It implements a minimal HTTP/1.0–1.1 request pipeline:

- **Socket layer**: accept TCP connections, read/write bytes (`socket/`)
- **Parser layer**: parse raw bytes into an `HttpRequest` (`parser/`)
- **Router layer**: map `(method, path)` → handler (`router/`)
- **Response layer**: build and serialize HTTP responses (`response/`)

The server listens on a port (default **8080**) and handles requests sequentially (one connection at a time).

## Routes / endpoints

Handlers live in `router/handlers.c`. Common routes include:

- `GET /` (index)
- `GET /echo` (echo query text)
- `POST /echo` (echo request body)
- `GET /headers` (dump request headers)
- `GET /health` (basic health check)

## Build and run on Linux (or WSL)

### Prerequisites

- A C compiler: `gcc` or `clang`
- CMake (3.10+)

On Ubuntu/Debian:

```bash
sudo apt update
sudo apt install -y build-essential cmake
```

### Build

From the repo root:

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j
```

This produces the executable `build/webserver`.

### Run

Default port 8080:

```bash
./build/webserver
```

Custom port:

```bash
./build/webserver 8000
```

Stop the server with `Ctrl+C`.