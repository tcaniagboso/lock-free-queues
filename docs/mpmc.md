# MPMC Queue

Lock-free bounded **Multi-Producer Multi-Consumer (MPMC)** queue implemented using a ring buffer, atomic indices, and per-slot sequence numbers.

---

## Overview

- Multiple producer threads push concurrently
- Multiple consumer threads pop concurrently
- No locks
- Producers coordinate on `tail`
- Consumers coordinate on `head`
- Slot ownership and readiness are managed using sequence numbers

---

## API Semantics

- `try_push` / `try_pop`
    - non-blocking
    - return immediately
    - may fail under contention or transient slot state mismatch
    - intended for benchmarking, polling loops, and fail-fast workloads

- `push` / `pop`
    - convenience operations
    - spin using `_mm_pause()` until the operation succeeds

---

## Core Idea

Each slot contains:

- `value`
- `sequence number`

Sequence numbers encode:

- whether a producer may write
- whether a consumer may read
- which reuse generation the slot belongs to

This removes the need for a separate `ready` flag.

---

## Slot Lifecycle

For a producer/consumer position `pos`:

```text
slot.seq == pos              -> producer may write
slot.seq == pos + 1          -> consumer may read
slot.seq == pos + capacity   -> next producer reuse cycle
```

Each slot advances through repeated producer and consumer turns.

---

## Push (Producers)

Producers reserve enqueue positions using `tail`.

```cpp
pos = tail.fetch_add(1)
index = pos % capacity

while (slot.seq != pos)
    pause

write value
slot.seq.store(pos + 1, release)
```

`try_push()` performs a fail-fast attempt:

```cpp
if queue appears full -> false
if slot not in producer phase -> false
if CAS on tail fails -> false
```

---

## Pop (Consumers)

Consumers reserve dequeue positions using `head`.

```cpp
pos = head.fetch_add(1)
index = pos % capacity

while (slot.seq != pos + 1)
    pause

read value
slot.seq.store(pos + capacity, release)
```

`try_pop()` performs a fail-fast attempt:

```cpp
if queue appears empty -> false
if slot not in consumer phase -> false
if CAS on head fails -> false
```

---

## Memory Ordering

- `head` / `tail` ownership counters:
  - `fetch_add(relaxed)` for blocking reservation paths
  - `compare_exchange_weak(acq_rel, relaxed)` for try paths 
  
- Slot synchronization:
  - `seq.load(acquire)` waits for correct slot phase
  - `seq.store(release)` publishes slot phase transition

This ensures safe handoff of data between producers and consumers.

---

## Performance Characteristics

- Non-blocking `try_push` / `try_pop` suitable for benchmarking and fail-fast semantics
- Convenience `push` / `pop` wrappers using spin-waiting until success
- Fully concurrent producer and consumer paths
- Sequence-number slots avoid separate ready flags
- Cache-line padded indices reduce false sharing
- Low latency under moderate contention
- Scales with multiple producers and consumers relative to lock-based designs

---

## Constraints

- **N** producers
- **M** consumers

All thread counts are supported.

---

## Notes

- `_mm_pause()` is used to reduce spin-wait overhead on x86
- Capacity should ideally be a power of two for faster modulo indexing
- `empty()`, `full()`, and `size()` are advisory snapshots under concurrency
- `push()` and `pop()` assume counterpart threads continue making progress

---

## Design Philosophy

This implementation separates:

- **ownership reservation** using `head` and `tail`
- **slot correctness** using per-slot sequence numbers

That makes the queue robust under multi-threaded contention while remaining lock-free.
