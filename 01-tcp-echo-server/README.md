# TCP Echo Server (Single Client, Sequential)

A server that accepts one client connection at a time, echoes back whatever the client sends, and returns to waiting for the next client once one disconnects — sequentially, never concurrently. A matching interactive client is included for testing.

**In scope:** blocking I/O, raw byte echo, one client served at a time (others wait to connect).
**Out of scope (deliberately):** multiple simultaneous clients and any protocol beyond raw bytes.

## Files

- `server.c` — the echo server
- `client.c` — an interactive terminal client for testing against it

## Build & Run

A Makefile is included:

```makefile
CC = gcc

# Warnings: All (turns on the most critical set of high probability bug detectors)
# -Wextra : Additional layer of stricter checks that -Wall leaves out
# -g tells gcc to embed debugging symbols inside the compiled executable. These symbols map machine instructions back to the human readable C source code, to be used while debugging (gdb or valgrind)
CFLAGS = -Wall -Wextra -g 

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client

.PHONY: all clean
```

```bash
make                 # builds both server and client
./server             # starts listening on port 8080
./client             # in a second terminal — connects and lets you type messages to echo

make clean           # removes both binaries
```

Type `exit` in the client to disconnect cleanly, or Ctrl+D for EOF, or Ctrl+C on the server to stop it (not a graceful shutdown — see Known Limitations).

## How it works

**Socket setup:** `socket()` creates an endpoint using `SOCK_STREAM` (TCP). `SO_REUSEADDR` is set so the server can restart immediately without hitting `EADDRINUSE` from a lingering `TIME_WAIT` socket from a previous run.

**Binding:** `bind()` attaches the socket to a specific local *address* — not just a port, but the IP + port pair together, which is exactly why `sockaddr_in` carries both `sin_addr` and `sin_port`. `INADDR_ANY` means "accept connections arriving on any local interface."

**Listening & accepting:** `listen()` marks the socket as passive and sets the backlog size — the maximum number of fully-established connections the kernel will hold in the queue waiting for your program to call `accept()`. Once that limit is reached, the kernel refuses additional connections rather than letting the queue grow unbounded — it's a hard cap, not just a wait threshold. Importantly, the kernel handles the entire TCP three-way handshake on its own, automatically, in the background — your program is not involved in that at all. `accept()`'s only job is to pull one already-completed connection off that queue; if the queue is empty, `accept()` simply blocks until the kernel adds one.

**The echo loop:** once a connection is accepted, `read()` blocks until data arrives (or the client disconnects), and each chunk read is immediately written back with `write()`. `read()` returning `0` means the client disconnected gracefully (sent a TCP FIN); returning `-1` means a real error.

**Sequential, not concurrent:** the whole `accept → serve → close(connfd)` sequence sits inside a `while(1)` loop, so after one client disconnects, the server goes back to `accept()` and waits for the next one. Two clients can never be served at the same time in this version — a second client connecting while the first is still active simply waits in the kernel's queue until the first is done.

## Error handling

Every syscall's return value is checked. A few decisions worth calling out explicitly:

- A failed `accept()` logs the error and loops to try the next connection — a single blip doesn't take the server down.
- A `read()` error or a `write()` error on one client's connection closes only that client's socket and moves on to the next `accept()` — one misbehaving client can't kill the server for everyone else.
- The final `close(sockfd)` at the bottom of `server.c` is currently unreachable dead code, since nothing in this version has a shutdown path out of `while(1)` — every exit from the loop is a `continue`. That's a known, accepted gap for this project's scope (see below), not an oversight.

## Known Limitations (by design, not bugs)

- **No graceful shutdown.** Stopping the server means killing the process (Ctrl+C) — there's no signal handler catching `SIGINT` to exit the loop cleanly and reach that final `close(sockfd)`. Signal handling is CS:APP Ch.8 territory (exceptional control flow), not part of this project's JIT scope (`socket`, `bind`, `listen`, `accept`, `read`/`write`) — deliberately deferred rather than missed.
- **No concurrency.** One client is served at a time, by design.
- **No real protocol.** Raw bytes only, no message framing or structure.