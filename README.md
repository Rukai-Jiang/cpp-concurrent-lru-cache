# C++ Concurrent LRU Cache

A header-only, thread-safe LRU cache written in modern C++20. It provides capacity-based eviction, optional per-entry TTLs, and runtime hit/miss metrics.

## Why this project

Caching combines backend fundamentals: hash maps, linked lists, concurrency control, expiration semantics, and operational metrics. The cache keeps `get` and `put` at average **O(1)** complexity while maintaining strict least-recently-used order.

## Features

- Thread-safe `get`, `put`, and `erase`
- Average O(1) lookup and update
- Optional time-to-live per entry
- Explicit expired-entry cleanup
- Hit, miss, eviction, and expiration counters
- Header-only template implementation
- Concurrent stress test

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Or directly:

```bash
c++ -std=c++20 -pthread -Iinclude tests/test_cache.cpp -o cache_tests
./cache_tests
```

## Design

An unordered map points to nodes in a doubly linked list. Recently accessed entries move to the front, while capacity eviction removes from the back. A single mutex protects the map, list, and statistics as one consistent state.
