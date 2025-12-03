# Concurrent Graph Processing Engine Using Read–Write Locks

### Final Project Report

---

## **1. Overview**

This project implements a high-performance, thread-safe graph engine supporting concurrent reads and writes using **Read–Write Locks** (`std::shared_mutex`). The goal is to maximize **parallel read access** while ensuring **exclusive write access** for operations that modify the graph’s structure.

The engine includes:

* Adjacency-list graph structure
* Thread-safe node + edge operations
* Concurrent BFS and DFS traversal
* Multi-threaded benchmark harness
* Performance comparison against a mutex-based implementation

Real-world use cases include:

* Database indexing
* Distributed systems
* Graph analytics
* Network topology traversal

---

## **2. Tasks Completed**

### **Task 1 – Graph Data Structure with RW-Locks**

Implemented adjacency list + RW synchronization using `shared_mutex`.

### **Task 2 – Traversal with Shared Read Locks**

Implemented BFS + DFS with shared locks for fully parallel traversals.

### **Task 3 – Multi-Threaded Testing Harness**

Created 16-thread benchmark:

* 80% reader threads
* 20% writer threads
* Randomized access patterns
* Latency + throughput measurement

### **Task 4 – RWLock vs Mutex Baseline Comparison**

Benchmarked:

* RWLock graph (`shared_mutex`)
* Mutex graph (`std::mutex`)

### **Task 5 – Written Analysis**

Provided detailed explanation of performance differences, trade-offs, and final conclusions.

---

## **3. Profiling Results (Task 3 Output)**

```
=== Test config: threads=16 readers%=80 duration=10s ===
Launching 13 reader threads and 3 writer threads.

=== Results ===
Elapsed: 10.0088 s
Read ops: 956184 | Write ops: 152446 | Total: 1108630
Throughput (ops/sec): 110765
Read throughput: 95534 ops/sec, avg latency = 0.135435 ms
Write throughput: 15231 ops/sec, avg latency = 0.195541 ms
Final nodes: 23645, approx edges: 1267
=================
```

---

## **4. Performance Summary Table**

| Metric             | Value               |
| ------------------ | ------------------- |
| Total Operations   | **1,108,630**       |
| Overall Throughput | **110,765 ops/sec** |
| Read Ops           | 956,184             |
| Read Throughput    | 95,534 ops/sec      |
| Avg Read Latency   | **0.135 ms**        |
| Write Ops          | 152,446             |
| Write Throughput   | 15,231 ops/sec      |
| Avg Write Latency  | **0.195 ms**        |
| Final Node Count   | 23,645              |
| Approx Edge Count  | 1,267               |

---

## **5. Task 5 – Written Analysis**

### **Read–Write Lock Performance**

The RW-lock implementation reached **110,765 ops/sec**, with readers accounting for ~95K ops/sec. Shared locks allow BFS/DFS traversals to run in parallel with minimal contention, producing low average read latency (0.135 ms). Writers require exclusive access, leading to moderately higher latency (0.195 ms), but the overall system remains highly efficient.

### **Mutex Performance**

A mutex-based graph forces serialization—each operation must wait for the single lock, destroying parallelism. This causes lower throughput and noticeably higher latency, especially under heavy read load.

### **Why RW-Locks Win at 80% Read Load**

* Readers do not block each other
* Traversals are frequent and lightweight
* Only 20% of operations require exclusive locking
* Shared reads dramatically reduce contention

### **Trade-offs**

* Writers may experience increased waiting time (possible starvation)
* More complex than a single mutex
* Overhead of managing shared/exclusive state

---

## **6. Conclusions**

RW-locks provide **major performance benefits** in read-heavy workloads, enabling concurrency and minimizing latency. The project successfully demonstrates why shared locking is the optimal strategy for graph search workloads dominated by reads.

---

## **7. How to Run**

```
g++ -std=c++17 task3.cpp -pthread -O2 -o graph_benchmark
./graph_benchmark
```

---

## **8. Repository Structure**

```
task1.cpp          – RWLock graph core
task2.cpp          – BFS/DFS traversal
task3.cpp          – Benchmark harness (80/20 load)
task4.cpp          – RWLock vs Mutex comparison
PROJECT_REPORT.md  – Final project report
```
