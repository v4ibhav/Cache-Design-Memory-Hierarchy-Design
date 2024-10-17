# Cache-Design-Memory-Hierarchy-Design

## ⚠️ WARNING: DO NOT COPY OR REDISTRIBUTE THIS CODE

This code is part of an academic submission and is monitored by the university's plagiarism detection system. Unauthorized copying, sharing, or distributing this code will result in academic misconduct and violation of the university's code of conduct.

Any unauthorized use will be flagged by the plagiarism checker, potentially leading to disciplinary actions.


---

## Project Overview

This project simulates a memory hierarchy design with configurable cache parameters, including L1 and L2 cache sizes, associativity, and a prefetcher. You can run the simulation with different configurations by following the steps below.

## Running the Code with Different Cache Configurations

### Step-by-Step Instructions

1. **Clean the project (optional but recommended)**:
   ```bash
   make clean

2. **Compile the project **:
   ```bash
   make all

3.
   ```bash
   ./sim 32 8192 4 262144 8 3 10 val-proj1-f22/gcc_trace.txt

### Cache Configuration Parameters

The numbers in the command correspond to the following cache configuration parameters:

- **BLOCKSIZE**: `32`
  (Block size in bytes for both L1 and L2 caches)

- **L1_SIZE**: `8192`
  (Size of L1 cache in bytes)

- **L1_ASSOC**: `4`
  (Associativity of the L1 cache)

- **L2_SIZE**: `262144`
  (Size of L2 cache in bytes, `0` means no L2 cache)

- **L2_ASSOC**: `8`
  (Associativity of the L2 cache)

- **PREF_N**: `3`
  (Number of blocks to prefetch)

- **PREF_M**: `10`
  (Prefetch distance, i.e., how far ahead to prefetch)

- **Trace File**: `val-proj1-f22/gcc_trace.txt`
  (The input trace file used to simulate memory accesses)

---

### Example Configuration

In the example command:

```bash
./sim 32 8192 4 262144 8 3 10 val-proj1-f22/gcc_trace.txt

