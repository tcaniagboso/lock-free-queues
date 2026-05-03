# SPSC Queue

Lock-free bounded **Single Producer Single Consumer (SPSC)** queue implemented using a ring buffer and atomic indices.

---

## Overview

- One producer thread writes to the queue
- One consumer thread reads from the queue
- No locks or compare-and-swap (CAS) required

The queue uses two monotonically increasing counters:

- `head` → next element to consume
- `tail` → next element to produce

---

## API Semantics

The queue provides two operation styles:

- `try_push` / `try_pop`
  - non-blocking
  - return immediately
  - useful for polling loops, fail-fast workloads, and explicit caller-controlled retry logic

- `push` / `pop`
  - blocking spin-wait operations
  - spin using `_mm_pause()` until the operation succeeds
  - useful for continuous producer-consumer pipelines where completion is expected

---

## Core Idea

- Producer owns `tail`
- Consumer owns `head`
- Each thread only writes to its own counter
- Threads coordinate using atomic loads with acquire/release semantics

---

## Push (Producer)

```cpp
head = head.load(acquire)
tail = tail.load(relaxed)

while (tail - head == capacity)
    pause

buffer[tail % capacity] = value
tail.store(tail + 1, release)
```

`try_push` performs the same check once and returns `false` if the queue is **full**.

---

## Pop (Consumer)

```cpp
head = head.load(relaxed)
tail = tail.load(acquire)

while (tail == head)
    pause

value = buffer[head % capacity]
head.store(head + 1, release)
```

`try_pop` performs the same check once and returns `false` if the queue is **empty**.

---

## Memory Ordering

- `acquire` on values written by the other thread
- `relaxed` on values owned by the current thread
- `release` when publishing updates

This ensures:

- producer sees completed reads before reusing slots
- consumer sees completed writes before reading data

---

## Performance Characteristics

- Non-blocking `try_push` / `try_pop` suitable for fail-fast semantics and caller-controlled retry logic
- Blocking `push` / `pop` operations optimized for continuous producer-consumer workloads
- No CAS operations required
- No write-side contention (single producer / single consumer ownership model)
- Cache-line padded indices reduce false sharing
- Very low latency and strong cache locality
- Well-suited for producer-consumer pipelines

---

## Constraints

- **1** producer
- **1** consumer

---

## Notes
- `_mm_pause()` is used to make spin-wait loops efficient on x86
- Capacity should ideally be a power of two for faster indexing