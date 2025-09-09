// utils/Graph.cc
#include "Graph.h"
#include <cstdio>
#include <algorithm>

Graph::Graph(int num_vars) : adj(num_vars ) {}

void Graph::addEdge(int v1, int v2) {
    if (v1 != v2) {
        adj[v1].insert(v2);
        adj[v2].insert(v1);
    }
}

std::vector<int> Graph::findCutset(int max_cutsize) {
    std::vector<int> cutset;
    // printf("Finding cutset with max size %d\n", max_cutsize);

    // 计算度数并排序
    std::vector<std::pair<int, int>> degrees; // (degree, variable)
    for (int v = 0; v < adj.size(); ++v) {
        degrees.emplace_back(adj[v].size(), v);
    }
    std::sort(degrees.begin(), degrees.end(), std::greater<>());

    // 标记哪些变量被加入割集
    std::vector<bool> is_cut(adj.size(), false);

    for (const auto& [deg, var] : degrees) {
        if (cutset.size() >= max_cutsize+1)
            break;

        cutset.push_back(var);
        is_cut[var] = true;

        if (!isConnected(is_cut)) {
            // printf("Graph disconnected with cutset size %zu\n", cutset.size());
            return cutset;
        }
    }

    // 没有断开连接
    return cutset;
}

bool Graph::isConnected(const std::vector<bool>& is_cut) {
    int n = adj.size();
    std::vector<bool> visited(n, false);

    // 找到一个非割集节点作为 DFS 起点
    int start = -1;
    for (int i = 0; i < n-1; i++) {
        if (!is_cut[i]) {
            start = i;
            // printf("Starting DFS from node %d\n", start);
            break;
        }
    }
    if (start == -1) return true; // 割集移除所有点，视为不连通

    // BFS or DFS
    std::function<void(int)> dfs = [&](int u) {
        visited[u] = true;
        for (int v : adj[u]) {
            if (!visited[v] && !is_cut[v])
                dfs(v);
        }
    };
    dfs(start);

    // 检查所有未在割集中的点是否都被访问
    for (int i = 0; i < n; ++i) {
        if (!is_cut[i] && !visited[i]){
            // printf("Node %d is unreachable\n", i);
            return false; // 存在不可达点 -> 图被断开了
        }
    }
    return true; // 图仍然连通
}

