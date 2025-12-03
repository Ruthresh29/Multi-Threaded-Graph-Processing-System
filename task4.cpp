#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <chrono>
#include <random>
#include <atomic>     
using namespace std;

class ConcurrentGraph_RW {
private:
    unordered_map<int, unordered_set<int>> adj;
    mutable shared_mutex rwlock;

public:
    void addNode(int n) {
        unique_lock lock(rwlock);
        adj[n];
    }

    void addEdge(int u, int v) {
        unique_lock lock(rwlock);
        adj[u].insert(v);
        adj[v].insert(u);
    }

    void bfs(int start) const {
        shared_lock lock(rwlock);
        if (!adj.count(start)) return;

        unordered_set<int> visited;
        queue<int> q;
        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj.at(u)) {
                if (!visited.count(v)) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }
    }
};

class ConcurrentGraph_Mutex {
private:
    unordered_map<int, unordered_set<int>> adj;
    mutable mutex mtx;

public:
    void addNode(int n) {
        lock_guard<mutex> lock(mtx);
        adj[n];
    }

    void addEdge(int u, int v) {
        lock_guard<mutex> lock(mtx);
        adj[u].insert(v);
        adj[v].insert(u);
    }

    void bfs(int start) const {
        lock_guard<mutex> lock(mtx);
        if (!adj.count(start)) return;

        unordered_set<int> visited;
        queue<int> q;
        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj.at(u)) {
                if (!visited.count(v)) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }
    }
};

struct Result {
    long long reads = 0;
    long long writes = 0;
    double read_latency_ms = 0;
    double write_latency_ms = 0;
};

template<typename GraphType>
Result benchmark(const string& name) {
    GraphType graph;

    for (int i = 0; i < 200; i++)
        graph.addNode(i);

    atomic<bool> running = true;     

    Result result;

    auto worker = [&](int id, bool isReader) {
        mt19937 rng(id + 1234);
        uniform_int_distribution<int> dist(0, 5000);

        while (running) {
            auto t1 = chrono::high_resolution_clock::now();

            if (isReader) {
                graph.bfs(dist(rng) % 200);
                auto t2 = chrono::high_resolution_clock::now();
                result.reads++;
                result.read_latency_ms += chrono::duration<double, milli>(t2 - t1).count();
            } else {
                int u = dist(rng);
                int v = dist(rng);
                graph.addNode(u);
                graph.addEdge(u, v);
                auto t2 = chrono::high_resolution_clock::now();
                result.writes++;
                result.write_latency_ms += chrono::duration<double, milli>(t2 - t1).count();
            }
        }
    };

    vector<thread> threads;
    int readers = 13, writers = 3;

    for (int i = 0; i < readers; i++)
        threads.emplace_back(worker, i, true);

    for (int i = 0; i < writers; i++)
        threads.emplace_back(worker, i + 100, false);

    this_thread::sleep_for(chrono::seconds(10));
    running = false;

    for (auto& t : threads)
        t.join();


    if (result.reads) result.read_latency_ms /= result.reads;
    if (result.writes) result.write_latency_ms /= result.writes;

    cout << "\n=== " << name << " ===\n";
    cout << "Read ops: " << result.reads << "\n";
    cout << "Write ops: " << result.writes << "\n";
    cout << "Avg read latency: " << result.read_latency_ms << " ms\n";
    cout << "Avg write latency: " << result.write_latency_ms << " ms\n";

    return result;
}

int main() {
    auto r1 = benchmark<ConcurrentGraph_RW>("RW-Lock Graph (shared_mutex)");
    auto r2 = benchmark<ConcurrentGraph_Mutex>("Mutex Graph (exclusive lock)");

    cout << "\n======= COMPARISON =======\n";
    cout << "RW-lock total ops : " << (r1.reads + r1.writes) << "\n";
    cout << "Mutex total ops    : " << (r2.reads + r2.writes) << "\n";
}
