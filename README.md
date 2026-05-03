# Lock-Free Queues & Benchmarking Suite

A C++20 project implementing and benchmarking bounded lock-free queue variants for low-latency concurrent systems.

This project explores low-latency concurrent data structures, memory ordering, cache behavior, false sharing, and API design for concurrent systems.

Implemented four bounded lock-free queue variants (**SPSC**, **MPSC**, **SPMC**, **MPMC**) with a configurable benchmark harness measuring throughput and latency across **1,700+** workloads.

Built to study real-world concurrency tradeoffs found in trading systems, messaging pipelines, schedulers, and low-latency infrastructure.

---

## Tech Stack

- **Language:** C++20
- **Build System:** CMake
- **Concurrency:** `std::atomic`, acquire/release semantics, lock-free queues
- **Threading:** `std::thread`
- **Performance:** cache-line padding, false-sharing mitigation
- **Benchmarking:** Custom latency / throughput harness with CSV export
- **Platform:** Linux / WSL, x86-64

---

## Features

**Implemented queue variants:**

- **SPSC**: Single Producer Single Consumer
- **MPSC**: Multi Producer Single Consumer
- **SPMC**: Single Producer Multi Consumer
- **MPMC**: Multi Producer Multi Consumer (sequence-numbered slots)

**Completed:**

- Unified benchmark framework
- CSV result export
- Try vs Blocking policy comparisons
- Mutex baseline comparisons

**Planned:**

- Unit tests
- Charts / visualization dashboards
- CPU affinity + reduced-noise benchmarking modes
- Additional queue variants / optimizations
- Evaluate sequence-numbered slots for **MPSC** and **SPMC** variants against the current ready-flag design

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
| MPMC  | Multiple producers, multiple consumers | `docs/mpmc.md` |

---

## API Philosophy

Each queue exposes two operation styles:

- `try_push` / `try_pop`
  - non-blocking
  - return immediately
  - useful for fail-fast workloads, polling loops, and explicit caller-controlled retry logic

- `push` / `pop`
  - blocking spin-wait operations
  - spin using `_mm_pause()` until the operation succeeds
  - useful for continuous producer-consumer pipelines where completion is expected
  - 
---

## Benchmarking

The benchmark suite measures:

- throughput
- average latency
- p50 / p95 / p999 latency
- min / max latency

Benchmark results are exported to CSV files in the `results/` folder.

---

## Benchmark Highlights

Benchmarks were run across payload sizes, queue capacities, operation counts, and varying producer / consumer thread configurations.  
The suite generated over **1,700+ benchmark runs** across all queue variants.

Headline results:

- **MPMC (Blocking)** achieved the highest observed throughput: **67.6M ops/sec**
- **SPSC** delivered the most consistent low-latency performance with minimal synchronization overhead
- **Blocking (`push/pop`) policies outperformed `try_push/try_pop` under contention**
- **Lock-free queues significantly outperformed mutex-based baselines** in multi-threaded workloads
- Throughput and tail latency varied meaningfully by queue topology, contention model, and payload size

Detailed CSV results are available in the `results/` folder.

---

## Benchmark Results (Representative Runs)

| Queue Type | Policy | Threads | Peak Throughput    | P99.9 Latency |
|------------|--------|---------|--------------------|---------------|
| SPSC       | Block  | 1P / 1C | ~36.0M ops/sec     | ~98 ns        |
| MPSC       | Block  | 8P / 1C | ~28M ops/sec       | ~350 ns       |
| SPMC       | Block  | 1P / 8C | ~24M ops/sec       | ~420 ns       |
| MPMC       | Block  | 8P / 8C | **~67.6M ops/sec** | ~490 ns       |
| Mutex      | Base   | 8P / 8C | ~1.2M ops/sec      | ~29,000+ ns   |

*Representative benchmark scenarios shown above. Full CSV datasets available in `/results`.*

---

## Core Technical Insights

- **Blocking Paradox:** Under contention, blocking `push/pop` often outperformed `try_` APIs by avoiding repeated CAS failures and reducing retry overhead.

- **OS Scheduling Tax:** Mutex baselines showed dramatically worse tail latency, demonstrating the cost of kernel scheduling, parking, and wakeups versus user-space spin waiting.

- **Mechanical Sympathy:** `PaddedAtomic` separates hot counters onto independent cache lines, reducing false sharing and coherence traffic.

---

## Build

```bash
mkdir build
cd build
cmake ..
make -j
```

---

## Run

**Benchmark all queues:**

```bash
./bench
```

**Benchmark specific queues:**

```bash
./bench --spsc
./bench --mpsc
./bench --spmc
./bench --mpmc
./bench --spsc --mpmc
```

**Help menu:**
```bash
./bench --help
```

---

## Project Structure

```text
lock-free-queues
├── include
│   ├── spsc
│   ├── mpsc
│   ├── spmc
│   ├── mpmc
│   └── utils
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
