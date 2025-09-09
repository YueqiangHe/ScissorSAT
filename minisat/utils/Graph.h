// utils/Graph.h
#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <unordered_set>

class Graph {
public:
    Graph(int num_vars);
    void addEdge(int v1, int v2);
    std::vector<int> findCutset(int max_cutsize = 7); // 返回一个小割集
    bool isConnected(const std::vector<bool>& is_cut); // 检查割集是否断开图的连接
    std::vector<std::unordered_set<int>> adj;
private:
    // std::vector<std::unordered_set<int>> adj;
};

#endif
