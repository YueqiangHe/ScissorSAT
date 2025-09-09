// minisat/core/GlobalDB.h
#ifndef Minisat_GlobalDB_h
#define Minisat_GlobalDB_h

#include "minisat/core/SolverTypes.h"
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace Minisat {

class GlobalClauseDB {
  std::mutex mtx;
  std::vector<vec<Lit>> clauses;
  std::unordered_map<uint64_t, bool> clauseHashes; // 用于去重
  const int LBD_THRESHOLD = 2;                     // 只共享高质量子句

  uint64_t computeClauseHash(const vec<Lit> &clause) {
    uint64_t hash = 0;
    for (int i = 0; i < clause.size(); i++) {
      hash = hash * 131 + (uint64_t)toInt(clause[i]);
    }
    return hash;
  }

public:
  void addClause(const vec<Lit> &clause, int lbd) {
    if (lbd > LBD_THRESHOLD)
      return;

    std::lock_guard<std::mutex> lock(mtx);
    uint64_t hash = computeClauseHash(clause);
    if (!clauseHashes.count(hash)) {
      clauses.push_back(clause);
      clauseHashes[hash] = true;
    }
  }

  void getClauses(std::vector<vec<Lit>> &out) {
    std::lock_guard<std::mutex> lock(mtx);
    out = clauses;
  }
};

class UnexploredPathStorage {
  std::mutex mtx;
  // 存储结构: 决策级别 -> 该级别未尝试的赋值
  std::unordered_map<int, std::vector<std::pair<Var, bool>>> paths;

public:
  void addPath(int decisionLevel, Var var, bool value) {
    std::lock_guard<std::mutex> lock(mtx);
    paths[decisionLevel].emplace_back(var, value);
  }

  std::vector<std::pair<Var, bool>> getPaths(int decisionLevel) {
    std::lock_guard<std::mutex> lock(mtx);
    auto it = paths.find(decisionLevel);
    if (it != paths.end()) {
      return it->second;
    }
    return {};
  }
};

} // namespace Minisat

#endif