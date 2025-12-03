#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <shared_mutex>
#include <mutex>

class ConcurrentGraph {
private:
    std::unordered_map<int, std::unordered_set<int>> adj;  
    std::shared_mutex rwlock;  

public:

    // Add node (write lock)
    void addNode(int node) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        if (adj.count(node) == 0) {
            adj[node] = std::unordered_set<int>();
        }
    }

    // Remove node (write lock)
    void removeNode(int node) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        if (adj.count(node)) {
            adj.erase(node);

            for (auto &p : adj) {
                p.second.erase(node);
            }
        }
    }

    // Add edge (write lock)
    void addEdge(int u, int v) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        if (!adj.count(u)) adj[u] = {};
        if (!adj.count(v)) adj[v] = {};

        adj[u].insert(v);
        adj[v].insert(u);
    }

    // Remove edge (write lock)
    void removeEdge(int u, int v) {
        std::unique_lock<std::shared_mutex> lock(rwlock);
        if (adj.count(u)) adj[u].erase(v);
        if (adj.count(v)) adj[v].erase(u);
    }

    // BFS traversal (read lock)
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

            for (int nbr : adj[node]) {
                if (!visited.count(nbr)) {
                    visited.insert(nbr);
                    q.push(nbr);
                }
            }
        }

        std::cout << "\n";
    }
};

int main() {
    ConcurrentGraph g;

    g.addNode(1);
    g.addNode(2);
    g.addNode(3);

    g.addEdge(1, 2);
    g.addEdge(2, 3);

    g.bfs(1);

    return 0;
}
