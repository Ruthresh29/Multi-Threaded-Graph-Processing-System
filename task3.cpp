

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <mutex>
#include <queue>
#include <stack>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <chrono>
#include <algorithm>

using namespace std;
using clk = chrono::high_resolution_clock;
using ns = chrono::nanoseconds;

class ConcurrentGraph {
private:
    unordered_map<int, unordered_set<int>> adj;
    mutable shared_mutex rwlock;
    atomic<int> nextNodeId{0};

public:
    ConcurrentGraph() = default;


    void init_nodes(int n) {
        unique_lock lock(rwlock);
        for (int i = 0; i < n; ++i) {
            adj[i]; 
        }
        nextNodeId.store(n);
    }


    int addNode() {
        unique_lock lock(rwlock);
        int id = nextNodeId.fetch_add(1);
        adj[id];
        return id;
    }

    bool removeNode(int u) {
        unique_lock lock(rwlock);
        if (!adj.count(u)) return false;
        adj.erase(u);
        for (auto &p : adj) p.second.erase(u);
        return true;
    }

    bool addEdge(int u, int v) {
        unique_lock lock(rwlock);
        if (!adj.count(u) || !adj.count(v)) return false;
        adj[u].insert(v);
        adj[v].insert(u);
        return true;
    }

    bool removeEdge(int u, int v) {
        unique_lock lock(rwlock);
        if (!adj.count(u) || !adj.count(v)) return false;
        adj[u].erase(v);
        adj[v].erase(u);
        return true;
    }

    int node_count() const {
        shared_lock lock(rwlock);
        return (int)adj.size();
    }

    int bfs(int start) const {
        shared_lock lock(rwlock);
        if (!adj.count(start)) return 0;
        unordered_set<int> visited;
        queue<int> q;
        visited.insert(start);
        q.push(start);
        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (int v : adj.at(u)) {
                if (!visited.count(v)) {
                    visited.insert(v);
                    q.push(v);
                }
            }
        }
        return (int)visited.size();
    }


    int dfs(int start) const {
        shared_lock lock(rwlock);
        if (!adj.count(start)) return 0;
        unordered_set<int> visited;
        stack<int> st;
        st.push(start);
        while (!st.empty()) {
            int u = st.top(); st.pop();
            if (visited.count(u)) continue;
            visited.insert(u);
            for (int v : adj.at(u)) {
                if (!visited.count(v)) st.push(v);
            }
        }
        return (int)visited.size();
    }

    long long total_edges() const {
        shared_lock lock(rwlock);
        long long s = 0;
        for (auto &p : adj) s += p.second.size();
        return s / 2; 
    }
};

struct Metrics {
    atomic<uint64_t> read_ops{0};
    atomic<uint64_t> write_ops{0};
    atomic<uint64_t> read_ns{0};  
    atomic<uint64_t> write_ns{0};  
};


void reader_worker(const ConcurrentGraph &g, Metrics &m, atomic<bool> &stop_flag, int thread_id, int max_start_node) {

    thread_local mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count() ^ (thread_id*6364136223846793005ULL));
    uniform_int_distribution<int> start_dist(0, max_start_node > 0 ? max_start_node : 0);
    uniform_int_distribution<int> pick_dist(0, 1); 

    while (!stop_flag.load()) {
        int start = start_dist(rng);
        int pick = pick_dist(rng);
        auto t0 = clk::now();
        if (pick == 0) g.bfs(start);
        else g.dfs(start);
        auto t1 = clk::now();
        uint64_t dt = (uint64_t)chrono::duration_cast<ns>(t1 - t0).count();
        m.read_ops.fetch_add(1);
        m.read_ns.fetch_add(dt);

        this_thread::sleep_for(chrono::microseconds(200));
    }
}

void writer_worker(ConcurrentGraph &g, Metrics &m, atomic<bool> &stop_flag, int thread_id) {
    thread_local mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count() ^ (thread_id*1442695040888963407ULL));
    uniform_int_distribution<int> op_dist(1, 4); 

    while (!stop_flag.load()) {
        int op = op_dist(rng);
        auto t0 = clk::now();
        bool ok = false;
        if (op == 1) {
            g.addNode();
            ok = true;
        } else if (op == 2) {
            int n = g.node_count();
            if (n > 0) {
                uniform_int_distribution<int> node_dist(0, n-1);
                ok = g.removeNode(node_dist(rng));
            }
        } else if (op == 3) {
            int n = g.node_count();
            if (n > 1) {
                uniform_int_distribution<int> node_dist(0, n-1);
                int u = node_dist(rng);
                int v = node_dist(rng);
                if (u != v) ok = g.addEdge(u, v);
            }
        } else {
            int n = g.node_count();
            if (n > 1) {
                uniform_int_distribution<int> node_dist(0, n-1);
                int u = node_dist(rng);
                int v = node_dist(rng);
                if (u != v) ok = g.removeEdge(u, v);
            }
        }
        auto t1 = clk::now();
        uint64_t dt = (uint64_t)chrono::duration_cast<ns>(t1 - t0).count();
        m.write_ops.fetch_add(1);
        m.write_ns.fetch_add(dt);

        this_thread::sleep_for(chrono::microseconds(500));
    }
}

void run_test(int initial_nodes = 200, int num_threads = 16, int reader_percent = 80, int duration_seconds = 10) {
    cout << "=== Test config: threads=" << num_threads
         << " readers%=" << reader_percent
         << " duration=" << duration_seconds << "s ===\n";

    ConcurrentGraph graph;
    graph.init_nodes(initial_nodes);


    {
        mt19937_64 rng(123456);
        uniform_int_distribution<int> d(0, initial_nodes-1);
        for (int i = 0; i < initial_nodes * 2; ++i) {
            int u = d(rng), v = d(rng);
            if (u != v) graph.addEdge(u, v);
        }
    }

    Metrics metrics;
    atomic<bool> stop_flag{false};

    vector<thread> workers;
    int reader_threads = (int)round(num_threads * (reader_percent / 100.0));
    reader_threads = max(1, reader_threads);
    int writer_threads = num_threads - reader_threads;
    writer_threads = max(1, writer_threads);

    cout << "Launching " << reader_threads << " reader threads and " << writer_threads << " writer threads.\n";

    // launch readers
    for (int i = 0; i < reader_threads; ++i) {
        workers.emplace_back(reader_worker, cref(graph), ref(metrics), ref(stop_flag), i, graph.node_count()-1);
    }
    // launch writers
    for (int i = 0; i < writer_threads; ++i) {
        workers.emplace_back(writer_worker, ref(graph), ref(metrics), ref(stop_flag), i + 100);
    }

    auto t0 = clk::now();
    this_thread::sleep_for(chrono::seconds(duration_seconds));
    stop_flag.store(true);
    for (auto &th : workers) if (th.joinable()) th.join();
    auto t1 = clk::now();

    double elapsed_s = chrono::duration_cast<chrono::duration<double>>(t1 - t0).count();
    uint64_t reads = metrics.read_ops.load();
    uint64_t writes = metrics.write_ops.load();
    uint64_t read_ns = metrics.read_ns.load();
    uint64_t write_ns = metrics.write_ns.load();

    cout << "\n=== Results ===\n";
    cout << "Elapsed: " << elapsed_s << " s\n";
    cout << "Read ops: " << reads << " | Write ops: " << writes << " | Total: " << (reads + writes) << "\n";
    cout << "Throughput (ops/sec): " << static_cast<uint64_t>((reads + writes) / elapsed_s) << "\n";
    if (reads) cout << "Read throughput: " << static_cast<uint64_t>(reads / elapsed_s) << " ops/sec, avg latency = "
                     << (read_ns / (double)reads) / 1e6 << " ms\n";
    if (writes) cout << "Write throughput: " << static_cast<uint64_t>(writes / elapsed_s) << " ops/sec, avg latency = "
                      << (write_ns / (double)writes) / 1e6 << " ms\n";

    cout << "Final nodes: " << graph.node_count() << ", approx edges: " << graph.total_edges() << "\n";
    cout << "=================\n\n";
}

int main(int argc, char** argv) {
    int initial_nodes = 200;
    int threads = 16;
    int read_pct = 80;
    int duration = 10;
    if (argc >= 2) initial_nodes = stoi(argv[1]);
    if (argc >= 3) threads = stoi(argv[2]);
    if (argc >= 4) read_pct = stoi(argv[3]);
    if (argc >= 5) duration = stoi(argv[4]);

    run_test(initial_nodes, threads, read_pct, duration);
    return 0;
}
