# Lock-Free Queues & Benchmarking Suite

A C++ project for implementing and benchmarking bounded lock-free queue variants.

This project explores low-latency concurrent data structures, memory ordering, cache behavior, false sharing, and API design for concurrent systems.

---

## Status

Work in progress.

Implemented queue variants:

- SPSC: Single Producer Single Consumer
- MPSC: Multi Producer Single Consumer
- SPMC: Single Producer Multi Consumer

Planned:

- MPMC: Multi Producer Multi Consumer using sequence-numbered slots
- Updated benchmark reports across all queue variants

---

## Goals

- Build lock-free queue implementations from scratch
- Compare different producer/consumer ownership models
- Study acquire/release memory ordering
- Understand cache-line padding and false sharing
- Benchmark latency and throughput under different workloads

---

## Queue Variants

| Queue | Description                            | Documentation  |
|-------|----------------------------------------|----------------|
| SPSC  | One producer, one consumer             | `docs/spsc.md` |
| MPSC  | Multiple producers, one consumer       | `docs/mpsc.md` |
| SPMC  | One producer, multiple consumers       | `docs/spmc.md` |
| MPMC  | Multiple producers, multiple consumers | In progress    |

---

## API Philosophy

Each queue exposes two operation styles:

- `try_push` / `try_pop`
  - non-blocking
  - return immediately
  - used for benchmarking and fail-fast workloads

- `push` / `pop`
  - convenience methods
  - spin using `_mm_pause()` until the operation succeeds

---

## Benchmarking

The benchmark suite measures:

- throughput
- average latency
- p50 / p95 / p999 latency
- min / max latency

Benchmark results are being updated as additional queue variants are implemented.

---

## Build

```bash
mkdir build
cd build
cmake ..
make
```

---

## Run

```bash
./queue_benchmark
```

---

## Project Structure

```text
lock-free-queues
├── include
│   └── lock_free
│       ├── spsc
│       ├── mpsc
│       ├── spmc
│       ├── mpmc
│       └── utils
├── benchmark
├── docs
├── src
├── tests
└── results
```

---

## Motivation

Lock-free queues are useful in high-performance systems where predictable latency, low synchronization overhead, and careful ownership boundaries matter.

This project is both a queue library and a learning-focused benchmarking environment for understanding practical concurrent programming in modern C++.
