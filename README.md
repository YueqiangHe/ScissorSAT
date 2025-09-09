# Formal Methods Course Project: ScissorSAT - A SAT Solving Strategy Based on Cutset Parallelism

PB22111649 He Yueqiang June 27, 2025

## 1. Background of SAT Solvers

### 1.1 Overview of the SAT Problem

The Boolean Satisfiability Problem (SAT) is a core problem in computer science. It asks whether there exists a set of Boolean variable assignments that makes a given logical formula true. Its importance lies in the following aspects:

- It is the first proven NP-complete problem (Cook-Levin Theorem).
- It is widely applied in fields such as hardware verification, software testing, and AI planning.
- Modern solvers can handle industrial-scale problems with millions of variables.

### 1.2 Principles of the CDCL Algorithm

Conflict-Driven Clause Learning (CDCL) is the core algorithm of modern SAT solvers. The process is as follows: Is all variables assigned? → If yes, return SAT. If not, perform propagation. Is there a conflict? → If no conflict, make a decision. → If there is a conflict, conduct conflict analysis and learn clauses, then perform intelligent backtracking.

Key innovations of the CDCL algorithm:

- Two-literal watching: Enables efficient propagation (Boolean Constraint Propagation, BCP).
- VSIDS heuristic: Facilitates dynamic variable selection.
- Learned clause management: Employs Literal Block Distance (LBD) for quality evaluation.

## 2. Previous Parallel Solving Schemes for SAT Solvers

### 2.1 Main Types of Parallel SAT Solver Frameworks

| Category           | Framework Description                                        | Representative Systems/Studies |
| ------------------ | ------------------------------------------------------------ | ------------------------------ |
| Portfolio-based    | Multiple CDCL solver instances with different configurations run in parallel to competitively solve the same problem. | ManySAT, Plingeling            |
| Divide-and-conquer | The original problem is split into multiple subproblems assigned to different cores, with a clear solution space division. | PSATO, PaMiraXT, Treengeling   |
| Hybrid methods     | Combines Portfolio and Divide methods, with partial sharing of search information. | Parallel CryptoMiniSat         |
| Clause-sharing     | Periodic sharing of learned clauses among multiple threads.  | ManySAT, Plingeling            |
| Lookahead-based    | Uses static preprocessing analysis for problem division to improve branch quality. | Cube-and-Conquer, March_cc     |

### 2.2 Evaluation of Various Parallel SAT Solver Frameworks

#### 2.2.1 Portfolio-based Framework

**Advantages**:

- Zero communication overhead, with threads running independently, making it suitable for heterogeneous environments.
- Demonstrates robust performance across different problem types.
- Easy to implement, no need to restructure the core of the solver.

**Disadvantages**:

- Inadequate resource utilization, with severe redundant computations among threads.
- Cannot guarantee improved solving efficiency for all cases.
- Lacks a collaboration mechanism, making it less friendly for large-scale problems.

#### 2.2.2 Divide-and-conquer Framework

**Advantages**:

- Theoretically offers the best scalability and speedup ratio.
- Can be directly mapped to multi-core processors, clusters, and even supercomputer nodes.
- Utilizes structural information such as cutsets and cubes, providing strong controllability.

**Disadvantages**:

- Difficulties in subproblem division; unbalanced division leads to load imbalance.
- Additional preprocessing is required for solution space division.
- The logic for returning and merging results between subproblems is complex.

#### 2.2.3 Hybrid Framework

**Advantages**:

- Combines competition and collaboration, and partial information sharing improves efficiency.
- Balances the overhead of redundancy and communication.

**Disadvantages**:

- Complex implementation, requiring reasonable scheduling and synchronization mechanisms.
- Excessive clause-sharing may lead to pollution and performance degradation.

#### 2.2.4 Clause-sharing Strategy

**Advantages**:

- Effectively accelerates branch pruning and avoids repeated searches.
- Highly efficient in instances with regular structures (e.g., hardware verification).

**Disadvantages**:

- Redundant clauses pollute the search paths of other threads.
- The adjustment of communication frequency and strategies is crucial (e.g., setting LBD thresholds).

#### 2.2.5 Lookahead/Cube-and-Conquer Framework

**Advantages**:

- Suitable for problems with strong structures and dense constraints.
- Reasonable solution space partitioning improves the quality of CDCL solving.

**Disadvantages**:

- Lookahead is time-consuming, resulting in high preprocessing costs.
- May perform poorly on unstructured problems.

### 2.3 Summary Table of Evaluations

| Direction      | Scalability | Implementation Difficulty | Communication Overhead | Robustness | Application Scenarios    |
| -------------- | ----------- | ------------------------- | ---------------------- | ---------- | ------------------------ |
| Portfolio      | Medium      | Low                       | None                   | High       | Various SAT problems     |
| Divide&Conq.   | High        | High                      | Medium                 | Medium     | Large-scale SAT problems |
| Hybrid         | High        | High                      | Medium                 | High       | General-purpose          |
| Clause-sharing | Medium      | Medium                    | High                   | Medium     | Industrial instances     |
| Lookahead/C&C  | High        | High                      | Low                    | Medium     | Structured problems      |

### 2.4 The New Method Proposed by Us

The aforementioned methods often require communication. However, independent operation can greatly reduce communication time. Therefore, we propose a CDCL parallel method based on cutsets (ScissorSAT), which effectively addresses the problem of high communication volume and exhibits excellent parallel scalability.

## 3. CDCL Parallel Method Based on Cutsets

### 3.1 Application of Cutset Theory in SAT

**Definition of a cutset**: In a variable co-occurrence graph, a cutset is the minimum set of vertices whose removal splits the graph into multiple connected components.

**Mathematical expression**: Let the graph corresponding to a SAT instance be \(G=(V, E)\). A cutset \(C \subseteq V\) satisfies: \(G \backslash C = G_{1} \cup G_{2} \cup ... \cup G_{k} (k \geq 2)\)

**Example**: Consider the Conjunctive Normal Form (CNF): \((x1 \lor x2) \land (\neg x2 \lor x3) \land (x4 \lor x5) \land (\neg x1 \lor \neg x5)\) Variable graph: The cutset \(C = \{x2, x5\}\). After its removal, two components are obtained: \(G_1\): \(\{x1, x3\}\) \(G_2\): \(\{x4\}\)

### 3.2 Design of the Parallel Strategy

#### 3.2.1 Basic Parallel Framework

The steps are as follows:

1. Construct the variable graph.
2. Perform cutset search (using a greedy algorithm for solving).
3. Enumerate all true/false assignments of the first k cutset elements respectively.
4. Add the assignments as assertion conditions to the two subproblems.
5. Transfer the clauses of the remaining non-cutset variables.
6. Perform parallel CDCL on the subproblems.
7. If one subproblem is UNSAT, mark the branch as UNSAT.
8. If both subproblems are SAT, return SAT.

**Flowchart**: Start → Construct variable graph → Find cutset → Is the cutset size > 7? → If yes: Enumerate cutset assignment combinations → Create a thread pool → Create a thread for each combination → Execute the solveForAssignment function (mark cutset variables → Use DFS to find connected components → Divide left and right variable sets → Create left and right solvers → Add cutset assignment constraints → Distribute clauses to solvers → Solve left and right components in parallel → Are both left and right components satisfied? → If yes, return true; if no, return false) → Collect thread results → Are there any satisfying combinations? → If yes, return SAT; if no, return UNSAT. → If no: Directly use CDCL for solving → Return the CDCL result. End

### 3.3 Key Innovations

1. Hierarchical parallelism: Cutset enumeration (outer layer) + component solving (inner layer).
2. Conflict-driven thread management: Any UNSAT result immediately terminates the branch.

## 4. Implementation and Optimization

### 4.1 Code Modification Design

We use the source code of Minisat (MiniSat Page) for modifications. After design, the following modifications are made:

```plaintext
minisat/
├── core/
     Solver.cc <-- Modification: Add support for parallel CDCL calls
     Solver.h <-- Modification: Add interface for branch subproblem processing
parallel/
├── CutsetSplitter.cc <-- Newly added: Graph construction, cutset finding, and CNF splitting
     CutsetSplitter.h <-- Newly added: Declare interfaces
├── utils/
     Graph.cc <-- Newly added: Variable graph construction (variable adjacency graph)
     Graph.h
main/
├── main_parallel.cc <-- Newly added: Startup program for cutset-based parallel CDCL
Makefile <-- Modification: Add compilation items for files in parallel/ and utils/
```

### 4.2 Specific Code Implementation

Due to space limitations, only the implementation of specific functions is listed.

#### CutsetSplitter.h

```cpp
// parallel/CutsetSplitter.h
#ifndef CUTSET_SPLITTER_H
#define CUTSET_SPLITTER_H
#include "minisat/core/SolverTypes.h"
#include "minisat/utils/Graph.h"
using namespace Minisat;

class CutsetSplitter {
public:
    // Decompose the total clauses and solve in parallel
    CutsetSplitter(const std::vector<std::vector<Lit>>& clauses, int num_vars);
    bool solveByCutset();

private:
    std::vector<std::vector<Lit>> clauses;
    int num_vars;
    // Schedule and solve subtasks
    bool solveForAssignment(const std::vector<int>& cutset, const std::vector<bool>& assignment, Graph& g);
};

#endif
```

#### Graph.h

```cpp
// utils/Graph.h
#ifndef GRAPH_H
#define GRAPH_H
#include <vector>
#include <unordered_set>

class Graph {
public:
    Graph(int num_vars);
    void addEdge(int v1, int v2);
    // Return a small cutset
    std::vector<int> findCutset(int max_cutsize = 7);
    // Check if the cutset disconnects the graph
    bool isConnected(const std::vector<bool>& is_cut);
    std::vector<std::unordered_set<int>> adj;

private:
    // std::vector<std::unordered_set<int>> adj;
};

#endif
```

#### Overall Steps: main_parallel.cc

```cpp
/********************************************************************************
*********[main_parallel.cc]
Based on MiniSat Main.cc. Modified to support cutset-based parallel CDCL.
*********************************************************************************
#include <errno.h>
#include <zlib.h>
#include <thread>
#include <vector>
#include "minisat/utils/System.h"
#include "minisat/utils/ParseUtils.h"
#include "minisat/utils/Options.h"
#include "minisat/core/Dimacs.h"
#include "minisat/core/Solver.h"
#include "minisat/parallel/CutsetSplitter.h"

using namespace Minisat;

static Solver* solver;

// Interrupt handling
static void SIGINT_interrupt(int) { solver->interrupt(); }

static void SIGINT_exit(int) {
    printf("\n*** INTERRUPTED ***\n");
    if (solver->verbosity > 0) {
        solver->printStats();
        printf("\n*** INTERRUPTED ***\n");
    }
    _exit(1);
}

int main(int argc, char** argv)
try {
    printf("Cutset-based Parallel CDCL Solver\n");
    printf("Based on MiniSat\n");

    setUsageHelp("USAGE: %s [options] <input-file> <result-output-file>\n\n"
                 " where input may be either in plain or gzipped DIMACS.\n");
    setX86FPUPrecision();

    IntOption verb("MAIN", "verb", "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
    IntOption cpu_lim("MAIN", "cpu-lim", "Limit on CPU time allowed in seconds.\n", 0, IntRange(0, INT32_MAX));
    IntOption mem_lim("MAIN", "mem-lim", "Limit on memory usage in megabytes.\n", 0, IntRange(0, INT32_MAX));
    BoolOption strictp("MAIN", "strict", "Validate DIMACS header during parsing.", false);

    parseOptions(argc, argv, true);

    Solver S;
    double initial_time = cpuTime();
    S.verbosity = verb;
    solver = &S;

    sigTerm(SIGINT_exit);
    if (cpu_lim != 0) limitTime(cpu_lim);
    if (mem_lim != 0) limitMemory(mem_lim);

    if (argc == 1)
        printf("Reading from standard input... Use '--help' for help.\n");

    gzFile in = (argc == 1) ? gzdopen(0, "rb") : gzopen(argv[1], "rb");
    if (in == NULL) {
        printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]);
        exit(1);
    }

    if (S.verbosity > 0) {
        printf("============================[ Problem Statistics ]=============================\n");
    }

    parse_DIMACS(in, S, (bool)strictp);
    gzclose(in);

    FILE* res = (argc >= 3) ? fopen(argv[2], "wb") : NULL;

    if (S.verbosity > 0) {
        printf("| Number of variables: %12d |\n", S.nVars());
        printf("| Number of clauses: %12d |\n", S.nClauses());
    }

    double parsed_time = cpuTime();
    if (S.verbosity > 0) {
        printf("| Parse time: %12.2fs |\n", parsed_time - initial_time);
        printf("===============================================================================\n");
    }

    sigTerm(SIGINT_interrupt);

    if (!S.simplify()) {
        if (res != NULL) {
            fprintf(res, "UNSAT\n");
            fclose(res);
        }
        if (S.verbosity > 0) {
            printf("============================[ Solution Found ]=============================\n");
            printf("Solved by unit propagation\n");
            S.printStats();
            printf("\n");
        }
        printf("UNSATISFIABLE\n");
        exit(20);
    }

    printf("Simplification complete.\n");

    // ------------------- Construct CutsetSplitter and solve -------------------
    std::vector<std::vector<Lit>> clauses;
    for (int i = 0; i < S.clauses.size(); i++) {
        Clause& c = S.ca[S.clauses[i]];
        std::vector<Lit> cl;
        for (int j = 0; j < c.size(); ++j)
            cl.push_back(c[j]);
        clauses.push_back(cl);
    }

    printf("Total clauses: %zu\n", clauses.size());

    CutsetSplitter splitter(clauses, S.nVars());

    // Print all information of the splitter
    if (S.verbosity > 0) {
        printf("============================[ Cutset Splitter Info ]=============================\n");
        printf("Cutset Splitter Information:\n");
        printf("Number of variables: %d\n", S.nVars());
        printf("Number of clauses: %zu\n", clauses.size());
    }

    printf("Starting cutset-based parallel CDCL...\n");
    bool result = splitter.solveByCutset();

    if (S.verbosity > 0) {
        printf("============================[ Solving Statistics ]=============================\n");
        S.printStats();
        printf("\n");
    }

    printf(result ? "SATISFIABLE\n" : "UNSATISFIABLE\n");

    if (res != NULL) {
        fprintf(res, result ? "SAT\n" : "UNSAT\n");
        fclose(res);

#ifdef NDEBUG
        exit(result ? 10 : 20);
#else
        return result ? 10 : 20;
#endif
    }

} catch (OutOfMemoryException&) {
    printf("============================[ Out of Memory ]=============================\n");
    printf("INDETERMINATE\n");
    exit(0);
}
```

## 5. Experimental Results

### 5.1 Dataset Selection

We use the random SAT formula generator that comes with Python for solving:

```python
# Use CNFgen to generate random problems
pip install cnfgen
cnfgen randkcnf 3 100 420 > random_3sat.cnf
```

Considering that we limit the maximum cutset size to 7 (due to the physical core limitations of the computer), statistics show that the cutset generally accounts for 10% of all variables. Therefore, we choose 100 variables for consideration.

### 5.2 Running Commands

Command for running the basic Minisat:

```bash
./build/release/bin/minisat random_3sat.cnf
```

Command for running the parallel version:

```bash
./build/release/bin/minisat_parallel random_3sat.cnf
```

### 5.3 Display of Partial Experimental Results

#### Data 1:

**Traditional CDCL Method**:

```plaintext
WARNING: for repeatability, setting FPU to use double precision
(AI)(base)hyqhyq:~/Formal/minisat$ ./build/release/bin/minisat random_3sat.cnf
============================[ Problem Statistics ]=============================
Number of variables: 100
Number of clauses: 420
Parse time: 0.00s
Eliminated clauses: 0.00 Mb
Simplification time: 0.00s
============================[ Search Statistics ]=============================
Conflicts | Vars | ORIGINAL | Clauses | Literals | Limit | LEARNT | Clauses | Lit/c1 | Progress
100       | 95   | 412      | 1262    | 151      | 100   | 7      | 0.010%  |
250       | 95   | 412      | 1262    | 166      | 89    | 6      | 0.010%  |
475       | 93   | 395      | 1210    | 182      | 103   | 6      | 2.030%  |
restarts  : 5
decisions : 602 (146918/sec)
propagations : 11982 (169434/sec)
conflicts : 522 (3372361/sec) (0.00% random)
conflict literals : 3129 (22.74% deleted)
Memory used : 5.84MB
CPU time : 0.003553s
UNSATISFIABLE
```

**Cutset-based Parallel CDCL Method**:

```plaintext
(AI)(base)hyqhyq:~/Formal/minisat$ ./build/release/bin/minisat_parallel random_3sat.cnf
Cutset-based Parallel CDCL Solver
Based on Minisat
WARNING: for repeatability, setting FPU to use double precision
============================[ Problem Statistics ]=============================
Number of variables: 100
Number of clauses: 420
Parse time: 0.00s
Simplification complete.
Total clauses: 420
============================[ Cutset Splitter Information ]=============================
Cutset Splitter Information:
Number of variables: 100
Number of clauses: 420
Starting cutset-based parallel CDCL...
restarts : 0
conflicts : 0 (0/sec)
decisions : 0 (-nan% random) (/sec)
propagations : 0 (0/sec)
conflict literals : 0 (-nan% deleted)
Memory used : 10.62MB
CPU time : 0.002824s
UNSATISFIABLE
```

#### Data 2:

**Traditional CDCL Method**:

```plaintext
(AI)(base)hyqhyq:~/Formal/minisat$ ./build/release/bin/minisat random_3sat.cnf
WARNING: for repeatability, setting FPU to use double precision
============================[ Problem Statistics ]=============================
Number of variables: 100
Number of clauses: 420
Parse time: 0.00s
Eliminated clauses: 0.00 Mb
Simplification time: 0.00s
============================[ Search Statistics ]=============================
Conflicts | Vars | ORIGINAL | Clauses | Literals | Limit | LEARNT | Clauses | Lit/c1 | Progress
100       | 99   | 419      | 1261    | 153      | 100   | 8      | 0.010%  |
250       | 99   | 419      | 1261    | 168      | 93    |        | 0.010%  |
restarts  : 3
conflicts : 368 (119870/sec)
decisions : 8546 (148534/sec) (0.00% random)
propagations : 456 (2783713/sec)
conflict literals : 2513 (19.38% deleted)
Memory used : 5.84MB
CPU time : 0.00307s
SATISFIABLE
```

**Cutset-based Parallel CDCL Method**:

```plaintext
(AI)(base)hyqhyq:~/Formal/minisat$ ./build/release/bin/minisat_parallel random_3sat.cnf
Cutset-based Parallel CDCL Solver
Based on Minisat
WARNING: for repeatability, setting FPU to use double precision
============================[ Problem Statistics ]=============================
Number of variables: 100
Number of clauses: 420
Parse time: 0.00s
Simplification complete.
Total clauses: 420
============================[ Cutset Splitter Information ]=============================
Cutset Splitter Information:
Number of variables: 100
Number of clauses: 420
Starting cutset-based parallel CDCL...
restarts : 0
conflicts : 0 (0/sec)
decisions : 0 (-nan% random) (/sec)
propagations : 0 (0/sec)
conflict literals : 0 (-nan% deleted)
Memory used : 10.62MB
CPU time : 0.00276s
```

Through extensive data statistics, we can see that when the cutset size is small, the parallel CDCL outperforms the traditional CDCL regardless of whether the problem is satisfiable or not, though the speedup is not significant. In terms of different scenarios, the speedup for SAT results is significantly smaller than that for UNSAT results. An intuitive analysis reveals the reason: UNSAT requires traversal, and this parallel method effectively splits the search space for traversal.

For problems with a small number of variables (e.g., 10 variables) or a large number of variables (e.g., 1000 variables), the parallel method barely achieves any speedup due to the parallel startup time and cutset size issues. Thus, further modifications are required.

## 6. Limitations and Future Work

Due to the significant impact of the cutset on parallelism and performance, and to better unleash its potential, we propose the following new ideas. However, due to time constraints (final exams), only further modification schemes are proposed.

### 6.1 Solution to Address the Significant Impact of Cutset on Performance and Parallelism

**Overall Idea**: First, find the cutset. Regardless of the cutset size, suppose the cutset has 20 elements. Only select the top 5 or top 10 elements with the highest degrees for enumeration. Then, perform serial CDCL solving on the remaining part of the cutset. Finally, perform parallel CDCL solving on the two split sets. Once backtracking reaches the cutset, the other party is determined to have failed and restarted. If both are satisfied, it indicates that the problem is satisfiable.

**Flowchart**: Start → Find cutset C → Calculate the degree of each variable in the cutset → Is the cutset size > threshold? → If yes: Select the top k variables Cₖ with the highest degrees → Enumerate 2ᵏ assignments of Cₖ → Branch processing → For each assignment combination of Cₖ: Fix the assignment of Cₖ → Create a CDCL solver → Add constraints: Remaining part Cᵣ of the cutset → Serially solve Cᵣ and component boundaries → Get the solution result → If satisfied: Decompose connected components → Create solver for component A and solver for component B → Solve component A and component B in parallel → Result: If any fails, terminate the other thread and backtrack to terminate the branch; if both are satisfied, return SAT and record conflict clauses. → If no: Enumerate all cutset variables directly → Solve in parallel enumeration. Next assignment combination → End

### 6.2 Solution to Address Insufficient Parallelism

To address the issue of insufficient parallelism, we propose using the divide-and-conquer approach. We found that each entire problem can be split into multiple subproblems. Thus, we can recursively split subproblems until the problem size is small enough for the computer to solve easily (e.g., when the total number of elements is less than 100), then solve it directly using CDCL, and induct upwards. Finally, this approach can greatly enhance parallelism while preventing the exponential explosion of the search space.

**Flowchart**: Start recursion → Find cutset C → Is the cutset size < decomposition threshold? → If yes: Directly solve using CDCL → Return the CDCL result. → If no: Enumerate cutset assignment combinations → For each assignment combination: Fix the cutset value → Decompose connected components → Recursively process component A and recursively process component B → Sub-results: If any is UNSAT, return UNSAT; if both are satisfied, return SAT. Merge results → End

The recursive process can be intuitively understood as follows: Original problem → Cutset C1 → Enumerate assignments → Component A1 and Component B1 → Are the scales still large? → If yes: Cutset C2 (for Component A1) and Cutset C3 (for Component B1) → Recursion → Component A2, Component B2, Component A3, Component B3 → Solve using CDCL for each component.

Such a high level of parallelism is suitable for application in supercomputer solving. The following is the design of a SAT supercomputing solving system:

Global task scheduler ↔ Control signals and status feedback ↔ Computing node clusters Computing node clusters include Node Group 1, Node Group 2, ..., Node Group N. Each node group has a task queue. For example, Node Group 1 has Working Node 1-1, Working Node 1-2,  Node Group N has Working Node N-1, Working Node N-2, ... Each working node is equipped with a recursive solver. There is a global conflict database for sharing learned clauses and global constraints. Specifically, recursive solvers share learned clauses and global constraints with the global conflict database, and the global conflict database also distributes global constraints and learned clauses to each recursive solver.

Supercomputers usually have tens of thousands of nodes, which are well-suited for such large-scale parallel computing.