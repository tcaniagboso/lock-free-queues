# SPMC Queue

Lock-free bounded **Single Producer Multi-Consumer (SPMC)** queue implemented using a ring buffer, atomic indices, and per-slot readiness flags.

---

## Overview

- One producer thread pushes into the queue
- Multiple consumer threads pop concurrently
- No locks
- Producer exclusively owns `tail`
- Consumers compete to advance `head`

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

The queue separates:

- **publish order** using `tail`
- **consumer ownership** using CAS on `head`
- **slot readiness** using per-slot `ready` flags

Each slot contains:

- `value`
- `ready`

This allows multiple consumers to race for the next item while preserving FIFO consumption order.

---

## Push (Producer)

The single producer owns `tail`, so no CAS is required.

```cpp
tail = tail.load(relaxed)
head = head.load(relaxed)

while (tail - head >= capacity)
    pause

index = tail % capacity

while (slot.ready == true)
    pause

write value
slot.ready.store(true, release)
tail.store(tail + 1, release)
```
`try_push()` performs the same checks once and returns `false` if the queue is **full** or the slot is not yet reusable.

---

## Pop (Consumers)

Consumers compete to claim the next index by advancing `head`.

```cpp
head = head.load(relaxed)
tail = tail.load(acquire)

while (head >= tail || !CAS(head, head + 1))
    pause

index = head % capacity

while (slot.ready == false)
    pause

read value
slot.ready.store(false, release)
```

`try_pop()` performs a fail-fast attempt:

```cpp
if empty -> false
if slot not ready -> false
if CAS fails -> false
```

---

## Memory Ordering

- `tail.store(..., release)` publishes newly produced items
- `tail.load(acquire)` allows consumers to observe producer progress
- `head.compare_exchange_weak(..., acq_rel, relaxed)` claims consumer ownership
- `slot.ready.store(true, release)` publishes slot contents
- `slot.ready.load(acquire)` ensures safe reads
- `slot.ready.store(false, release)` marks slot reusable

---

## Performance Characteristics

- Non-blocking `try_push` / `try_pop` suitable for fail-fast semantics and caller-controlled retry logic
- Blocking `push` / `pop` operations optimized for continuous producer-consumer workloads
- Single producer avoids enqueue-side CAS contention
- Multi-consumer dequeue path coordinated via CAS on `head`
- FIFO consumption order
- Cache-line padded indices reduce false sharing
- Low latency under moderate consumer contention
- Scales with multiple consumers relative to lock-based designs

---

## Constraints

- **1** producer
- **N** consumers

Using multiple producers is undefined behavior.

---

## Notes
- `_mm_pause()` is used to reduce spin-wait overhead on x86
- Capacity should ideally be a power of two for faster modulo indexing
- `try_pop()` may return false even when another consumer succeeds concurrently



