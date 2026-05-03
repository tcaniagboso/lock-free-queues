# MPSC Queue

Lock-free bounded **Multi-Producer Single Consumer (MPSC)** queue implemented using a ring buffer, atomic indices, and per-slot readiness flags.

---

## Overview

- Multiple producer threads push concurrently
- One consumer thread pops
- No locks
- Producers coordinate using an atomic `tail`
- Consumer exclusively owns `head`

---

## API Semantics

- `try_push` / `try_pop`
  - non-blocking
  - return immediately
  - may fail under contention or transient slot unavailability
  - useful for polling loops, fail-fast workloads, and explicit caller-controlled retry logic

- `push` / `pop`
  - blocking spin-wait operations
  - spin using `_mm_pause()` until the operation succeeds
  - useful for continuous producer-consumer pipelines where completion is expected

---

## Core Idea

The queue separates two concepts:

- **slot reservation** → handled by `tail`
- **data publication** → handled by each slot’s `ready` flag

Each slot contains:

- `value`
- `ready` flag

---

## Push (Producers)

`push()` uses `fetch_add` to reserve a unique slot and is optimized for sustained multi-producer throughput:

```cpp
head = head.load(acquire)
tail = tail.load(relaxed)

while (tail - head >= capacity)
    pause

pos = tail.fetch_add(1, relaxed)

while (pos - head.load(acquire) >= capacity)
    pause

index = pos % capacity

while (slot.ready == true)
    pause

write value
slot.ready.store(true, release)
```

`try_push()` uses CAS on tail and returns `false` if the **slot** cannot be claimed immediately.

---

## Pop (Consumer)

```cpp
head = head.load(relaxed)
tail = tail.load(acquire)

if (tail == head) → empty

index = head % capacity

if slot.ready == false → not ready

read value
slot.ready.store(false, release)
head.store(head + 1, release)
```

`pop()` spins until data is available.
`try_pop()` returns `false` if the queue is **empty** or the reserved slot is **not ready** yet.

---

## Memory Ordering

- `fetch_add(relaxed)` → unique slot assignment
- `compare_exchange_weak(..., acq_rel, relaxed)` → fail-fast slot claim in `try_push`
- `ready.store(true, release)` → publishes producer data
- `ready.load(acquire)` → consumer safely observes published data
- `head.load(acquire)` → producer observes completed consumer progress

---

## Performance Characteristics

- Non-blocking `try_push` / `try_pop` suitable for fail-fast semantics and caller-controlled retry logic
- Blocking `push` / `pop` operations optimized for continuous producer-consumer workloads
- `push()` optimized for sustained multi-producer throughput using `fetch_add`
- `try_push()` optimized for immediate return using CAS-based slot claims
- Per-slot readiness flags reduce producer-consumer coordination overhead
- Cache-line padded indices reduce false sharing
- Low latency under moderate contention
- Scales well with multiple producers relative to lock-based designs

---

## Constraints

- **N** producers
- **1** consumer

Using multiple consumers is undefined behavior.

---

## Notes
- `_mm_pause()` is used to make spin-wait loops efficient on x86
- Capacity should ideally be a power of two for faster indexing

