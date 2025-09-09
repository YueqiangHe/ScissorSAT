// parallel/CutsetSplitter.h
#ifndef CUTSET_SPLITTER_H
#define CUTSET_SPLITTER_H
#include "minisat/core/SolverTypes.h"
#include "minisat/utils/Graph.h"

using namespace Minisat;

class CutsetSplitter {
public:
    CutsetSplitter(const std::vector<std::vector<Lit>>& clauses, int num_vars);
    bool solveByCutset();
private:
    std::vector<std::vector<Lit>> clauses;
    int num_vars;
    bool solveForAssignment(const std::vector<int>& cutset, const std::vector<bool>& assignment , Graph& g);
};

#endif
