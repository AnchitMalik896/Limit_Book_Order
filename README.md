# Limit Order Book

A high-performance C++20 limit order book implementing price-time priority matching with a shared-nothing multi-threaded architecture. The engine supports multiple order types, deterministic matching, and is designed to demonstrate low-latency systems programming, concurrent programming, and efficient data structure design.

---

## Features

- Price-Time Priority (FIFO) matching
- Multi-threaded shared-nothing architecture
- Multi-symbol support
- Order types
  - Limit
  - Market
  - IOC (Immediate-Or-Cancel)
  - FOK (Fill-Or-Kill)
  - GTC (Good-Till-Cancelled)
- Partial fills
- Order cancellation
- Deterministic order matching
- Lock-free communication using SPSC queues
- Modular C++20 design

---

## Architecture

```
                    +----------------+
                    | Order Generator|
                    +-------+--------+
                            |
                            |
                 Gateway Message Queue
                            |
             +--------------+--------------+
             |                             |
      +------+-------+              +------+-------+
      | Symbol Worker|              | Symbol Worker|
      |    Thread    |              |    Thread    |
      +------+-------+              +------+-------+
             |                             |
             +--------------+--------------+
                            |
                    Trade Execution
```

Each symbol is assigned to a dedicated worker thread.

This shared-nothing design eliminates contention between symbols and allows independent processing while preserving deterministic price-time priority within each order book.

---

## Data Structures

- Price levels stored using ordered containers
- FIFO queues maintain time priority
- O(1) order lookup for cancellation
- Lock-free Single Producer Single Consumer queues
- Symbol-to-worker routing table

---

## Build

Requirements

- C++20
- GNU Make
- GCC / Clang

Compile

```bash
make
```

Run

```bash
./lob
```

---

## Supported Order Types

| Order Type | Supported |
|------------|-----------|
| Limit      | ✅ |
| Market     | ✅ |
| IOC        | ✅ |
| FOK        | ✅ |
| GTC        | ✅ |
| Cancel     | ✅ |
| Partial Fill | ✅ |

---

## Benchmark

Benchmark suite is currently being developed for the multi-threaded implementation.

Future reports will include:

- Throughput (Orders/sec)
- Average latency
- p50 / p95 / p99 / p99.9 latency
- Scalability across worker threads

---

## Repository Structure

```
src/
├── exchange.cpp
├── exchange.h
├── generator.cpp
├── gateway_msg.h
├── main.cpp
├── order.h
├── orderbook.cpp
├── orderbook.h
├── spsc_queue.h
├── symbol_table.h
└── trade.h
```

---

## Future Improvements

- Comprehensive benchmarking framework
- Unit and integration tests
- Performance profiling
- Market data feed simulation
- Persistent order logging
- Replay engine
- Additional execution algorithms

---

## License

This project is intended for educational and portfolio purposes.
