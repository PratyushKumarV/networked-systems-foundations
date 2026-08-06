# Concurrent Echo Server (Thread-per-Client)

Extends the sequential TCP echo server to handle many clients at once, using one detached thread per connection.

**In scope:** thread-per-client concurrency, safely passing per-connection data across the thread boundary.
**Out of scope (deliberately):** any shared state between clients — this server has none, so there's nothing to synchronize yet.

## Files

- `server.c` — the concurrent echo server
- `client.c` — the same interactive test client from Project 1, unchanged

## Build & Run

```bash
make                  # requires -pthread in CFLAGS — see Makefile
./server
```

Open several terminals running `./client` at once to see concurrent handling in action.

## Design

**Main thread stays minimal — all per-client work moved into `handle_client()`.** The `accept()` loop's only job is: accept a connection, hand it off to a new thread, and immediately go back to `accept()` for the next one. It never blocks waiting for a client to finish.

**Per-connection data is heap-allocated, not passed by stack address.** `connfd` and `client_addr` are both local variables in the main loop, reused (or at least not guaranteed to persist) on every iteration. Passing a pointer to either directly into the new thread would create a race: the main loop could overwrite that stack slot with the *next* client's data before the new thread has read it. Instead, a `client_args` struct is `malloc`'d per connection, populated with a full copy of the connection info, and the pointer to *that* is what gets passed to `pthread_create()`. The thread frees it once it's done reading from it.

**Threads are detached, not joined.** `pthread_join()` would block the caller until the target thread finishes — exactly the blocking behavior the main loop needs to avoid. `pthread_detach()` tells the kernel to reap the thread's resources automatically the moment it exits, with nobody needing to wait on it.

**`pthread_create()`'s error convention is different from the syscalls used everywhere else in this project.** Syscalls like `accept()`/`read()`/`write()` return `-1` and set `errno` on failure, so `perror()` works. `pthread_create()` instead returns the error code directly as its return value and never touches `errno` — so error reporting here uses `strerror(ret)` on the returned value, not `perror()`.

## Error handling

Same fd-leak-free discipline as Project 1, now inside `handle_client()`: a write failure and a read error are distinguished (not conflated into a false "graceful disconnect" report), and `close(connfd)` runs on every exit path from the thread, paired with `free()` on the heap-allocated args struct.

If `pthread_create()` itself fails (e.g. hitting the OS's thread limit), the main loop closes that connection's fd, frees the args struct that was already allocated for it, logs the real reason via `strerror()`, and continues accepting future connections rather than crashing the server.

## Known Limitations (by design, not bugs)

- **Interleaved output.** With multiple clients connected simultaneously, `printf`/`perror` output from different threads can interleave unpredictably in the terminal — individual calls won't corrupt (glibc's stdio is thread-safe), but there's no ordering guarantee across threads. Not a bug, just what unsynchronized concurrent logging looks like.
- **No shared state, on purpose.** Every client is handled in complete isolation — no counters, no shared cache, nothing requiring a lock. That's the next project's material, not this one's.
- **No limit on total threads spawned.** A high volume of simultaneous clients could exhaust system resources (each thread has real memory/kernel overhead). Bounding concurrency is what a thread *pool* solves — deliberately out of scope here.