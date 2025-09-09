/*****************************************************************************************[main_parallel.cc]
    Based on MiniSat Main.cc. Modified to support cutset-based parallel CDCL.
**************************************************************************************************/

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

// 中断处理
static void SIGINT_interrupt(int) { solver->interrupt(); }
static void SIGINT_exit(int) {
    printf("\n*** INTERRUPTED ***\n");
    if (solver->verbosity > 0){
        solver->printStats();
        printf("\n*** INTERRUPTED ***\n"); }
    _exit(1);
}

int main(int argc, char** argv)
{
    try {
        printf("Cutset-based Parallel CDCL Solver\n");
        printf("Based on MiniSat\n");
        setUsageHelp("USAGE: %s [options] <input-file> <result-output-file>\n\n"
                     "  where input may be either in plain or gzipped DIMACS.\n");
        setX86FPUPrecision();

        IntOption    verb   ("MAIN", "verb",   "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
        IntOption    cpu_lim("MAIN", "cpu-lim","Limit on CPU time allowed in seconds.\n", 0, IntRange(0, INT32_MAX));
        IntOption    mem_lim("MAIN", "mem-lim","Limit on memory usage in megabytes.\n", 0, IntRange(0, INT32_MAX));
        BoolOption   strictp("MAIN", "strict", "Validate DIMACS header during parsing.", false);

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
        if (in == NULL){
            printf("ERROR! Could not open file: %s\n", argc == 1 ? "<stdin>" : argv[1]);
            exit(1);
        }

        if (S.verbosity > 0){
            printf("============================[ Problem Statistics ]=============================\n");
            printf("|                                                                             |\n");
        }

        parse_DIMACS(in, S, (bool)strictp);
        gzclose(in);

        FILE* res = (argc >= 3) ? fopen(argv[2], "wb") : NULL;

        if (S.verbosity > 0){
            printf("|  Number of variables:  %12d                                         |\n", S.nVars());
            printf("|  Number of clauses:    %12d                                         |\n", S.nClauses());
        }

        double parsed_time = cpuTime();
        if (S.verbosity > 0){
            printf("|  Parse time:           %12.2f s                                       |\n", parsed_time - initial_time);
            printf("|                                                                             |\n");
        }

        sigTerm(SIGINT_interrupt);
        // printf("===============================================================================\n");

        if (!S.simplify()){
            if (res != NULL) fprintf(res, "UNSAT\n"), fclose(res);
            if (S.verbosity > 0){
                printf("===============================================================================\n");
                printf("Solved by unit propagation\n");
                S.printStats();
                printf("\n");
            }
            printf("UNSATISFIABLE\n");
            exit(20);
        }
        printf("Simplification complete.\n");

        // ------------------- 构建 CutsetSplitter 并求解 -------------------
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
        // 打印splitter的所有信息
        if (S.verbosity > 0) {
            printf("===============================================================================\n");
            printf("Cutset Splitter Information:\n");
            printf("Number of variables: %d\n", S.nVars());
            printf("Number of clauses: %zu\n", clauses.size());
        }

        printf("Starting cutset-based parallel CDCL...\n");
        bool result = splitter.solveByCutset();

        if (S.verbosity > 0) {
            printf("===============================================================================\n");
            S.printStats();
            printf("\n");
        }

        printf(result ? "SATISFIABLE\n" : "UNSATISFIABLE\n");

        if (res != NULL) {
            fprintf(res, result ? "SAT\n" : "UNSAT\n");
            fclose(res);
        }

#ifdef NDEBUG
        exit(result ? 10 : 20);
#else
        return result ? 10 : 20;
#endif

    } catch (OutOfMemoryException&) {
        printf("===============================================================================\n");
        printf("INDETERMINATE\n");
        exit(0);
    }
}
