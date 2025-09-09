// parallel/CutsetSplitter.cc
#include "CutsetSplitter.h"
#include "minisat/core/Solver.h"
#include <thread>
#include <functional>

CutsetSplitter::CutsetSplitter(const std::vector<std::vector<Lit>>& cls, int nv)
    : clauses(cls), num_vars(nv) {}

bool CutsetSplitter::solveByCutset() {
    Graph g(num_vars);
    for (const auto& clause : clauses) {
        for (size_t i = 0; i < clause.size(); ++i)
            for (size_t j = i+1; j < clause.size(); ++j){
                g.addEdge(var(clause[i]), var(clause[j]));
                // printf("Adding edge between %d and %d\n", var(clause[i]), var(clause[j]));
            }
    }

    auto cutset = g.findCutset();
    //打印cutset中的所有变量
    // printf("Cutset variables: ");
    // for (const auto& v : cutset) {
    //     printf("%d ", v);
    // }
    // printf("\n");
    // printf("Cutset size: %zu\n", cutset.size());
    //直接使用CDCL求解
    if (cutset.size() > 7){
        // printf("Cutset size %zu exceeds limit, using CDCL directly.\n", cutset.size());
        Solver solver;
        for (int i = 0; i < num_vars; ++i) {
            solver.newVar();
        }
        for (const auto& clause : clauses) {
            Minisat::vec<Lit> minisat_clause;
            for (const auto& lit : clause) {
                minisat_clause.push(lit);
            }
            solver.addClause(minisat_clause);
        }
        bool res = solver.solve();
        return res;
    }

    int total = 1 << cutset.size();
    printf("Trying %d assignments for cutset of size %zu\n", total, cutset.size());
    // for (int mask = 0; mask < total; ++mask) {
    //     std::vector<bool> assign(cutset.size());
    //     for (int i = 0; i < cutset.size(); ++i){
    //         assign[i] = (mask >> i) & 1;
    //         //打印所有组合方式
    //         // printf("Combination for cutset variable %d: %d\n", cutset[i], static_cast<int>(assign[i]));
    //     }
    //     bool ok = solveForAssignment(cutset, assign , g);
    //     if (ok) return true; // 有一个组合可满足
    // }
    // 利用生产者消费者模式并行处理所有组合
    std::vector<std::thread> threads;
    std::vector<bool> results(total, false);
    for (int mask = 0; mask < total; ++mask) {
        threads.emplace_back([&, mask]() {
            std::vector<bool> assign(cutset.size());
            for (int i = 0; i < cutset.size(); ++i) {
                assign[i] = (mask >> i) & 1;
            }
            bool ok = solveForAssignment(cutset, assign, g);
            results[mask] = ok;
            // if (ok) {
            //     printf("Found satisfying assignment for mask %d\n", mask);
            // }
        });
    }
    for (auto& t : threads) {
        t.join();
    }
    for (const auto& res : results) {
        if (res) return true;
    }
    return false;
}

bool CutsetSplitter::solveForAssignment(const std::vector<int>& cutset, const std::vector<bool>& assign, Graph& g) {
    // Step 1: 标记割集
    std::unordered_set<int> cutset_set(cutset.begin(), cutset.end());
    int n_vars = num_vars; // 使用成员变量 num_vars

    // Step 2: 找到除去割集后的一个连通片（DFS）
    std::vector<bool> visited(n_vars, false);
    std::vector<int> left_vars, right_vars;

    // 找到一个非割集变量作为起点
    int start = -1;
    for (int i = 0; i < n_vars; ++i) {
        if (!cutset_set.count(i)) {
            start = i;
            break;
        }
    }

    // DFS 构造左边变量集合
    std::function<void(int)> dfs = [&](int u) {
        visited[u] = true;
        left_vars.push_back(u);
        for (int v : g.adj[u]) {
            if (!visited[v] && !cutset_set.count(v)) {
                dfs(v);
            }
        }
    };
    dfs(start);

    // Step 3: 剩下的非割集变量归为右边变量
    for (int i = 0; i < n_vars; ++i) {
        if (!visited[i] && !cutset_set.count(i)) {
            right_vars.push_back(i);
        }
    }

    // Step 4: 割集变量加入左边和右边变量
    for (int v : cutset) { left_vars.push_back(v); right_vars.push_back(v); }

    // //打印左边和右边变量
    // printf("Left variables: ");
    // for (int v : left_vars) {
    //     printf("%d ", v);
    // }
    // printf("\nRight variables: ");
    // for (int v : right_vars) {
    //     printf("%d ", v);
    // }
    // printf("\n");

    // Step 5: 构造两个 Solver
    Solver solver_left, solver_right;
    int max_var = n_vars;
    for (int i = 0; i < max_var; ++i) {
        solver_left.newVar();
        solver_right.newVar();
    }

    // Step 6: 加入赋值的单文字子句
    for (int i = 0; i < cutset.size(); ++i) {
        int var = cutset[i];
        bool val = assign[i];
        Lit lit = mkLit(var, !val); // true → 正文字，false → 否文字
        solver_left.addClause(lit);
        solver_right.addClause(lit);
    }

    // Step 7: 分发子句
    for (const auto& clause : clauses) {
        bool in_left = true, in_right = true;
        for (const auto& lit : clause) {
            int v = var(lit);
            if (cutset_set.count(v)) continue;
            if (std::find(left_vars.begin(), left_vars.end(), v) == left_vars.end())
                in_left = false;
            if (std::find(right_vars.begin(), right_vars.end(), v) == right_vars.end())
                in_right = false;
        }

        // Convert std::vector<Lit> to Minisat::vec<Lit>
        Minisat::vec<Lit> minisat_clause;
        for (const auto& lit : clause) {
            minisat_clause.push(lit);
        }

        if (in_left) solver_left.addClause(minisat_clause);
        if (in_right) solver_right.addClause(minisat_clause);
    }

    // Step 8: 分别 CDCL 求解
    // bool sat_left = solver_left.solve();
    // bool sat_right = solver_right.solve();
    // 使用多线程并行求解
    bool sat_left = false, sat_right = false;
    std::thread t1([&]() { sat_left = solver_left.solve(); });
    std::thread t2([&]() { sat_right = solver_right.solve(); });
    t1.join();
    t2.join();

    return sat_left && sat_right;
}

