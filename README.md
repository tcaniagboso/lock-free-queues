# Lock-Free SPSC Queue (C++)

This project implements a **bounded lock-free single-producer single-consumer (SPSC) queue** in modern C++.  

The goal is to explore low-latency concurrent data structures and understand the practical effects of memory ordering, cache behavior, and contention.

The queue is implemented using C++ atomics and a ring buffer, with careful attention to minimizing synchronization overhead and avoiding false sharing.

---

## Features

- Lock-free **bounded ring buffer queue**
- **Single Producer / Single Consumer** concurrency model
- **Acquire / Release memory ordering**
- **Cache-line alignment (`alignas(64)`)** to prevent false sharing
- **Producer/consumer microbenchmark**
- Latency statistics:
  - Average latency
  - p50 / p95 / p999 latency
  - Min / Max latency
- Throughput measurement

---

## Queue Design

The queue uses two atomic counters:

- head → next element to be popped
- tail → next element to be pushed
    
A fixed-size ring buffer stores the elements.  
Head and tail counters grow monotonically and are mapped into the buffer using modulo indexing.

Producer thread:
- write value → update tail (release)

Consumer thread:
- read value → update head (release)

Acquire loads ensure proper synchronization between producer and consumer without using locks.

---

## Benchmark

A simple microbenchmark measures:

- Producer enqueue latency
- Consumer dequeue latency
- Overall throughput

Each operation records latency using `std::chrono::steady_clock`.

Statistics reported:

- average latency
- p50
- p95
- p999
- min / max

Due to clock resolution limits, extremely fast operations may appear as `0 ns`.

Example output:
```bash
Throughput: 2.7e+07 ops/sec

PUSH:
  Avg latency (ns): 49
  p50 latency (ns): 0
  p95 latency (ns): 109
  p999 latency (ns): 109

POP:
  Avg latency (ns): 50
  p50 latency (ns): 0
  p95 latency (ns): 109
  p999 latency (ns): 216
```
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
./benchmark <capacity> <num_push> <num_pop>
```

Example

```bash
./benchmark 1024 1000000 1000000
```

Parameters:

```markdown
capacity   → queue capacity
num_push   → number of enqueue operations
num_pop    → number of dequeue operations
```
---

## Project Structure

```markdown
lock-free-spsc-queue
│
├── CMakeLists.txt
├── README.md
│
├── include
│   └── spsc_queue.hpp
│
├── src
│   └── main.cpp
│
└── build
```
---

## Future Work

Planned improvements:
- CPU affinity for more stable benchmarking
- Warmup phase for microbenchmarks
- Spin-wait statistics
- Comparison against mutex-based queues
- Multi-producer multi-consumer (MPMC) queue implementation
    
---

## Motivation

Understanding lock-free data structures and memory ordering is essential for building **high-performance concurrent systems**, including:

- trading infrastructure
- low-latency messaging systems
- task schedulers
- real-time pipelines
    
This project serves as an exploration of implementing and benchmarking lock-free concurrent structures in modern C++.
