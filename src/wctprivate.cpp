#include "wctprivate.h"
#include <fmt/core.h>
#include <boost/timer/timer.hpp>
#include <limits>
#include <memory>
#include <vector>
#include "BranchBoundTree.hpp"
#include "Instance.h"
#include "LocalSearch_new.h"
#include "PricerSolverArcTimeDP.hpp"
#include "PricerSolverBddBackward.hpp"
#include "PricerSolverBddForward.hpp"
#include "PricerSolverSimpleDP.hpp"
#include "PricerSolverZddBackward.hpp"
#include "PricerSolverZddForward.hpp"
#include "PricingStabilization.hpp"
#include "Solution.hpp"
#include "Statistics.h"

Problem::~Problem() = default;

Problem::Problem(int argc, const char** argv)
    : parms(argc, argv),
      stat(parms),
      instance(parms),
      tree(),
      root_pd(std::make_unique<NodeData>(this)),
      status(no_sol),
      opt_sol() {
    /**
     *@brief Finding heuristic solutions to the problem or start without
     *feasible solutions
     */
    stat.start_resume_timer(Statistics::heuristic_timer);
    if (parms.use_heuristic) {
        heuristic();
    } else {
        Sol best_sol(instance);
        best_sol.construct_edd(instance.jobs);
        fmt::print("Solution Constructed with EDD heuristic:\n");
        best_sol.print_solution();
        best_sol.canonical_order(instance.intervals);
        fmt::print("Solution in canonical order: \n");
        best_sol.print_solution();
        opt_sol = best_sol;
    }

    stat.suspend_timer(Statistics::heuristic_timer);
    /**
     * @brief Build DD at the root node
     *
     */
    stat.start_resume_timer(Statistics::build_dd_timer);
    switch (parms.pricing_solver) {
        case bdd_solver_simple:
            root_pd->solver = std::make_unique<PricerSolverBddSimple>(instance);
            break;
        case bdd_solver_cycle:
            root_pd->solver = std::make_unique<PricerSolverBddCycle>(instance);
            break;
        case zdd_solver_cycle:
            root_pd->solver = std::make_unique<PricerSolverZddCycle>(instance);
            break;
        case zdd_solver_simple:
            root_pd->solver = std::make_unique<PricerSolverSimple>(instance);
            break;
        case bdd_solver_backward_simple:
            root_pd->solver =
                std::make_unique<PricerSolverBddBackwardSimple>(instance);
            break;
        case bdd_solver_backward_cycle:
            root_pd->solver =
                std::make_unique<PricerSolverBddBackwardCycle>(instance);
            break;
        case zdd_solver_backward_simple:
            root_pd->solver =
                std::make_unique<PricerSolverZddBackwardSimple>(instance);
            break;
        case zdd_solver_backward_cycle:
            root_pd->solver =
                std::make_unique<PricerSolverZddBackwardCycle>(instance);
        case dp_solver:
            root_pd->solver = std::make_unique<PricerSolverSimpleDp>(instance);
            break;
        case ati_solver:
            root_pd->solver = std::make_unique<PricerSolverArcTimeDp>(instance);
            break;
        case dp_bdd_solver:
            root_pd->solver = std::make_unique<PricerSolverSimpleDp>(instance);
            break;
        default:
            root_pd->solver =
                std::make_unique<PricerSolverBddBackwardCycle>(instance);
            break;
    }
    stat.suspend_timer(Statistics::build_dd_timer);
    stat.first_size_graph = root_pd->solver->get_nb_vertices();

    /**
     * @brief Initial stabilization method
     *
     */
    auto* tmp_solver = root_pd->solver.get();
    switch (parms.stab_technique) {
        case stab_wentgnes:
            root_pd->solver_stab = std::make_unique<PricingStabilizationStat>(
                tmp_solver, root_pd->pi);
            break;
        case stab_dynamic:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationDynamic>(tmp_solver,
                                                              root_pd->pi);
            break;
        case stab_hybrid:
            root_pd->solver_stab = std::make_unique<PricingStabilizationHybrid>(
                tmp_solver, root_pd->pi);
            break;
        case no_stab:
            root_pd->solver_stab = std::make_unique<PricingStabilizationBase>(
                tmp_solver, root_pd->pi);
            break;
        default:
            root_pd->solver_stab = std::make_unique<PricingStabilizationStat>(
                tmp_solver, root_pd->pi);
            break;
    }

    root_pd->solver->update_UB(opt_sol.tw);
    stat.root_upper_bound = opt_sol.tw + instance.off;
    stat.global_upper_bound = opt_sol.tw + instance.off;

    /**
     * @brief Initialization of the B&B tree
     *
     */
    stat.start_resume_timer(Statistics::bb_timer);
    tree = std::make_unique<BranchBoundTree>(std::move(root_pd), 0, 1);
    tree->explore();
    stat.global_upper_bound = static_cast<int>(tree->get_UB()) + instance.off;
    stat.global_lower_bound = static_cast<int>(tree->get_LB()) + instance.off;
    stat.rel_error = (stat.global_upper_bound - stat.global_lower_bound) /
                     (EPS + stat.global_lower_bound);
    stat.suspend_timer(Statistics::bb_timer);
    to_csv();
}

NodeData::~NodeData() = default;
void Problem::solve() {
    tree->explore();
    if (parms.print) {
        to_csv();
    }
}

void Problem::heuristic() {
    auto                         ILS = instance.nb_jobs / 2;
    auto                         IR = parms.nb_iterations_rvnd;
    boost::timer::auto_cpu_timer timer_heuristic;

    Sol best_sol{instance};
    best_sol.construct_edd(instance.jobs);
    fmt::print("Solution Constructed with EDD heuristic:\n");
    best_sol.print_solution();
    best_sol.canonical_order(instance.intervals);
    fmt::print("Solution in canonical order: \n");
    best_sol.print_solution();

    /** Local Search */
    auto local = LocalSearchData(instance.nb_jobs, instance.nb_machines);
    local.RVND(best_sol);
    /** Perturbation operator */
    PerturbOperator perturb{};

    best_sol.canonical_order(instance.intervals);
    fmt::print("Solution after local search:\n");
    best_sol.print_solution();
    root_pd->add_solution_to_colpool(best_sol);

    Sol sol{instance};
    int iterations = 0;
    for (auto i = 0UL; i < IR; ++i) {
        Sol sol1{instance};
        sol1.construct_random_shuffle(instance.jobs);
        sol = sol1;

        for (auto j = 0UL; j < ILS; ++j) {
            local.RVND(sol1);
            ++iterations;
            if (sol1.tw < sol.tw) {
                sol = sol1;
                j = 0UL;
            }
            perturb(sol1);
        }

        if (sol.tw < best_sol.tw) {
            best_sol = sol;
            root_pd->add_solution_to_colpool(sol);
        }
    }

    fmt::print("Best new heuristics\n");
    best_sol.print_solution();
    opt_sol = best_sol;
}
