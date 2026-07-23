# Networked Systems Foundations

The progression of small, focused projects that built the two foundations everything later depends on: a working multithreaded HTTP proxy, and a persistent key-value store that becomes the entry point into the distributed-systems work beyond it. Each project adds exactly one new concept on top of the last.

These aren't meant to stand alone as portfolio pieces. They're deliberate scaffolding — every folder below is small enough to finish quickly, and each one is a direct prerequisite for something further down the path. See [`http-caching-proxy`](LINK_HERE) and [`distributed-rpc-framework`](LINK_HERE) for the two projects this repo actually leads to.

## Why bundled into one repo

Each of these took an afternoon to a few days, not weeks — bundling them here is an honest reflection of what they are: stepping stones, not standalone systems. The full story, in order, is below.

## The Progression

| # | Project | Adds | Builds On |
|---|---------|------|-----------|
| 1 | [`01-tcp-echo-server`](./01-tcp-echo-server) | `socket()`/`bind()`/`listen()`/`accept()`, blocking I/O | — |
| 2 | [`02-concurrent-echo-server`](./02-concurrent-echo-server) | Process or thread creation per client | 01 |
| 3 | [`03-simple-kv-store`](./03-simple-kv-store) | A minimal line-based protocol, hash-map storage | 01 |
| 4 | [`04-static-file-http-server`](./04-static-file-http-server) | HTTP/1.0 parsing, MIME types, path safety | 01 |
| 5 | [`05-thread-pool`](./05-thread-pool) | Mutexes, condition variables, producer-consumer | 02 |
| 6 | [`06-multithreaded-http-server`](./06-multithreaded-http-server) | Thread pool wired into a real HTTP server | 04, 05 |
| 7 | [`07-event-driven-kv-store`](./07-event-driven-kv-store) | `epoll`, the reactor pattern | 03 |
| 8 | [`08-persistent-kv-store`](./08-persistent-kv-store) | Write-ahead logging, crash recovery | 07 |
| 9 | [`09-event-driven-web-server`](./09-event-driven-web-server) | Reactor pattern applied to file-serving at C10K scale | 04, 07 |

Project 6 is the direct prerequisite for the [caching HTTP proxy](LINK_HERE) — the first project in this journey meant to be judged on its own merits. Project 8 is the other real endpoint: it's a hard prerequisite for the [RPC framework](LINK_HERE) that opens up the entire Distributed Core branch (MapReduce, Raft, replicated and sharded storage). Project 9 is the one exception — a standalone comparison exercise (thread-per-connection vs. event-loop) with nothing depending on it further.

## Repo structure

```
networked-systems-foundations/
├── 01-tcp-echo-server/
│   └── README.md
├── 02-concurrent-echo-server/
│   └── README.md
├── 03-simple-kv-store/
│   └── README.md
├── 04-static-file-http-server/
│   └── README.md
├── 05-thread-pool/
│   └── README.md
├── 06-multithreaded-http-server/
│   └── README.md
├── 07-event-driven-kv-store/
│   └── README.md
├── 08-persistent-kv-store/
│   └── README.md
└── 09-event-driven-web-server/
    └── README.md
```

Each subfolder has its own short README: scope (in/out), what it builds on, and what specifically broke or was tricky to get right — even at this small scale, the failure mode is usually the more interesting part.

## Running any of these

```bash
cd 0X-project-name
make            # or: gcc *.c -o server -lpthread
./server <port>
```

(Adjust per-project — see each subfolder's README for exact build/run instructions and dependencies.)

## Part of a larger path

This repo is one stage in a longer progression toward distributed systems and system design mastery. See [`systems-mastery`](LINK_HERE) for the full roadmap and links to every later stage.
