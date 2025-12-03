#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <vector>

class ConcurrentGraph {
private:
    std::unordered_map<int, std::unordered_set<int>> adj;
    std::shared_mutex rwlock;

public:

    // ----------------------------
    // WRITE OPERATIONS (exclusive)
    // ----------------------------

    void addNode(int node) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        adj[node];  // creates empty set if missing
    }

    void addEdge(int u, int v) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        adj[u].insert(v);
        adj[v].insert(u);
    }

    void removeNode(int node) {
        std::unique_lock<std::shared_mutex> lock(rwlock);

        if (!adj.count(node)) return;

        adj.erase(node);

        for (auto &p : adj) {
            p.second.erase(node);
        }
    }

    void removeEdge(int u, int v) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        adj[u].erase(v);
        adj[v].erase(u);
    }

    // ----------------------------
    // READ OPERATIONS (shared)
    // ----------------------------

    // BFS using shared read lock
    void bfs(int start) {
        std::shared_lock<std::shared_mutex> lock(rwlock);

        if (!adj.count(start)) {
            std::cout << "Start node not found\n";
            return;
        }

        std::unordered_set<int> visited;
        std::queue<int> q;

        q.push(start);
        visited.insert(start);

        while (!q.empty()) {
            int node = q.front();
            q.pop();

            std::cout << node << " ";

            for (int nbr : adj.at(node)) {
                if (!visited.count(nbr)) {
                    visited.insert(nbr);
                    q.push(nbr);
                }
            }
        }

        std::cout << "\n";
    }

    // DFS helper
    void dfsUtil(int node, std::unordered_set<int> &visited) {
        visited.insert(node);
        std::cout << node << " ";

        for (int nbr : adj.at(node)) {
            if (!visited.count(nbr)) {
                dfsUtil(nbr, visited);
            }
        }
    }

    // DFS using shared read lock
    void dfs(int start) {
        std::shared_lock<std::shared_mutex> lock(rwlock);

        if (!adj.count(start)) {
            std::cout << "Start node not found\n";
            return;
        }

        std::unordered_set<int> visited;
        dfsUtil(start, visited);
        std::cout << "\n";
    }
};


// --------------------------------------------
// MAIN — Demonstrates MULTITHREADED TRAVERSALS
// --------------------------------------------

int main() {
    ConcurrentGraph g;

    // Build a sample graph
    g.addNode(1);
    g.addNode(2);
    g.addNode(3);
    g.addNode(4);

    g.addEdge(1, 2);
    g.addEdge(2, 3);
    g.addEdge(3, 4);
    g.addEdge(1, 4);

    std::cout << "Running BFS/DFS from multiple threads...\n";

    std::thread t1([&] { g.bfs(1); });
    std::thread t2([&] { g.dfs(1); });
    std::thread t3([&] { g.bfs(2); });
    std::thread t4([&] { g.dfs(3); });

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    return 0;
}
