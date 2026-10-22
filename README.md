# Parallel Prime Number Finder

A case study for **Parallel and Distributed Computing**, written in C++.

## Problem

Finding all prime numbers in a large range takes a long time when one thread checks every number.

## Solution

The program uses **data parallelism**: the range is divided into equal chunks, and each chunk is processed by its own thread at the same time. The main thread waits for all threads to finish, then combines the results.

## Features

- Input a number range
- Find prime numbers using multiple threads
- Divide the range among threads
- Display results and the work done by each thread
- Compare processing time between sequential and parallel runs

## Concepts Used

- Threads (`std::thread`)
- Task division and data parallelism
- Parallel execution
- Synchronization (`join()` and a mutex for console output)
- Shared resources
- Load distribution

## How to Run

Requires a C++ compiler (g++ with C++17).

```
g++ -O2 -std=c++17 -pthread src/main.cpp -o prime_finder.exe
./prime_finder.exe
```

Menu options:

1. Live demo: shows each thread's range, result, and time
2. Benchmark: compares 2, 4, 8, 12, and 16 threads

## Test Environment

- Laptop: (model)
- CPU logical processors: 12
- Compiler: g++ (version) with -O2

## Results (Range: 1 to (your range))

| Threads        | Time (ms) | Speedup |
| -------------- | --------- | ------- |
| 1 (sequential) | (value)   | 1.00x   |
| 2              | (value)   | (value) |
| 4              | (value)   | (value) |
| 8              | (value)   | (value) |
| 12             | (value)   | (value) |
| 16             | (value)   | (value) |

## Analysis

- Speedup grows as threads are added, until it reaches the hardware limit.
- Beyond the number of physical cores, extra threads add overhead and give little or no gain.
- Equal ranges do not mean equal work: larger numbers take longer to check, so the last thread finishes later (load imbalance).
- For small ranges, thread creation overhead can make the parallel version no faster than sequential.

## Limitations and Possible Improvements

- Better load balancing, such as interleaved or smaller chunks
- A faster prime algorithm, such as the Sieve of Eratosthenes

## Setup (Windows)

1. Download the repo: click **Code, then Download ZIP**, and extract it
   to a folder path without spaces (for example `C:\dev\parallel-prime-finder`).
2. Install **MSYS2** (msys2.org). In the **MSYS2 UCRT64** terminal run:
   `pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain`
3. Add `C:\msys64\ucrt64\bin` to your Windows PATH, then open a new terminal
   and check with `g++ --version`.
4. In the project folder, compile and run:

```
   g++ -O2 -std=c++17 -pthread src/main.cpp -o prime_finder.exe
   .\prime_finder.exe
```

5. Test with option 2 using range `1` to `100000`. Expected: **9592 primes**.
