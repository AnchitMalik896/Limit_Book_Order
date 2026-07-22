# Project Overview

This repository implements a C++20 multi-threaded Limit Order Book.

## Constraints

- C++20 only.
- Standard Library only.
- No Boost.
- No external dependencies.
- Preserve correctness.
- Preserve architecture.
- Shared-nothing design.
- One matching thread per symbol.
- Lock-free SPSC queues.

## Build

Always build after making changes.

Never leave compilation errors.

## Coding Style

- Prefer minimal changes.
- Reuse existing code.
- Avoid duplicate implementations.
- Explain every architectural decision.

## Performance

Whenever changing performance-sensitive code:

- Measure throughput.
- Measure latency.
- Compare before/after.

Never claim performance improvements without benchmarking.

## Output

Always provide:

- Files modified
- Why they changed
- Complexity impact