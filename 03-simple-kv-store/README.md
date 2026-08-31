# Simple Key-Value Store over TCP

A single-threaded TCP server implementing a small text protocol — `SET key value`, `GET key`, `DEL key` — backed by an in-memory hash table.

**In scope:** designing a minimal line-based protocol, a basic hash table (separate chaining), single-client sequential handling.
**Out of scope (deliberately):** concurrency, persistence — data is gone the moment the server restarts, on purpose.

## Files

- `server.c` — the KV store

## Build & Run

```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g

all: server

server: server.c
	$(CC) $(CFLAGS) -o server server.c

clean:
	rm -f server

.PHONY: all clean
```

```bash
make
./server
```

Test with `nc 127.0.0.1 8080` or your own client, typing commands like:
```
SET name pratyush
GET name
DEL name
GET name
```

## Protocol

Newline-delimited, space-separated, case-insensitive command names:

| Command | Response |
|---|---|
| `SET <key> <value>` | `OK` |
| `GET <key>` | `VALUE <value>` or `NOT FOUND` |
| `DEL <key>` | `DELETED` or `NOT FOUND` |
| missing key | `NO KEY PROVIDED` |
| `SET` with no value | `NO VALUE PROVIDED` |
| unrecognized command | `ERROR` |

## How it works

**Storage:** an array of SIZE (1031) bucket heads, each a singly-linked list of {key, value} nodes — classic separate chaining. 1031 is prime, which spreads hash values more evenly across buckets than a round number would. SET traverses the chain first to update existing keys in place—freeing the previous value allocation—rather than appending duplicate nodes.

**Hashing:** djb2 — seed at `5381`, then for each character, `hash = hash*33 + character`, letting the multiplication overflow naturally. This is intentional, not a bug: the running hash is stored in a fixed-width unsigned integer, and as more characters get folded in across a long string, the true mathematical value grows far past what that type can hold. In C, unsigned overflow wraps around predictably rather than being undefined behavior — that wraparound is what keeps the hash bounded while still scrambling bits unpredictably as the string is consumed. The bucket index is `hash % SIZE`.

**Parsing:** `strtok()` splits each line on spaces/newline into up to three tokens — command, key, value. `value` is legitimately `NULL` when a client sends `GET`/`DEL` (which don't take one); `key` being `NULL` means the client sent no arguments at all.

**Memory ownership — why `strdup()` matters here:** `key` and `value` inside `process()` point into the connection's stack-allocated `buff`, which gets overwritten on the *next* `read()` call. `SET` uses `strdup()` to allocate independent heap copies before storing them in the hash table — without this, every stored key/value would become garbage the moment the client sent its next command. `DEL` correspondingly frees both strings before freeing the node itself, avoiding a leak.

## Error handling

- Missing `key` is checked once, immediately after parsing, before the hash/index/dispatch logic ever touches it — since `key` being `NULL` is only possible when `token` (the command itself) was also `NULL` (empty input line), this one check catches both cases.
- `SET` additionally checks for a missing `value` before ever calling `strdup(value)`.
- Both checks `return` unconditionally after responding — not conditioned on whether the response itself was sent successfully — so malformed input can't fall through into `hash(NULL)` or `strdup(NULL)`, either of which is undefined behavior.
- An unrecognized command gets an explicit `ERROR` response rather than being silently dropped, so a client is never left blocked waiting on a reply that will never come.

## Known Limitations (by design, not bugs)

- **One command per `read()` call, not per line.** TCP is a byte stream with no built-in message boundaries — if a client pipelines multiple newline-separated commands into a single `write()` (e.g. `"SET a 1\nSET b 2\n"` sent together), only the first line currently gets parsed and processed; the rest of that read's buffer is silently dropped rather than deferred to the next iteration. Manual, one-line-at-a-time testing (`nc`, typing interactively) never triggers this, since each line naturally arrives in its own `read()`. Proper stream framing — buffering partial lines and handling multiple complete commands within one read — is a deliberately deferred boundary for this project, not an oversight; revisit if a later project's protocol depends on it.
- **No persistence.** Everything lives in memory; a restart wipes the store clean.
- **No concurrency.** One client at a time, sequential — same model as Project 1, not yet extended with threads.