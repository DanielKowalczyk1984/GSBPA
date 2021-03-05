#include "wctprivate.h"
#include <fmt/core.h>
#include <algorithm>
#include <boost/timer/timer.hpp>
#include <limits>
#include <memory>
#include <type_traits>
#include <vector>
#include "LocalSearch_new.h"
#include "PricerSolverArcTimeDP.hpp"
#include "PricerSolverBddBackward.hpp"
#include "PricerSolverBddForward.hpp"
#include "PricerSolverSimpleDP.hpp"
#include "PricerSolverZddBackward.hpp"
#include "PricerSolverZddForward.hpp"
#include "PricingStabilization.hpp"
#include "Solution_new.hpp"
// #include "interval.h"

Problem::~Problem() { /*free the parameters*/
    g_ptr_array_free(g_job_array, TRUE);
    g_ptr_array_free(intervals, TRUE);
    solution_free(&(opt_sol));
}

Problem::Problem(int argc, const char** argv)
    : parms(),
      stat(),
      tree(),
      g_job_array(g_ptr_array_new_with_free_func(g_job_free)),
      intervals(g_ptr_array_new_with_free_func(g_interval_free)),
      opt_sol(nullptr),
      nb_jobs(),
      nb_machines(),
      p_sum(),
      pmax(),
      pmin(std::numeric_limits<int>::max()),
      dmax(std::numeric_limits<int>::min()),
      dmin(std::numeric_limits<int>::max()),
      off(),
      H_min(),
      H_max(std::numeric_limits<int>::max()),
      /*B&B info*/
      global_upper_bound(std::numeric_limits<int>::max()),
      global_lower_bound(),
      rel_error(std::numeric_limits<double>::max()),
      root_lower_bound(),
      root_upper_bound(std::numeric_limits<int>::max()),
      root_rel_error(std::numeric_limits<double>::max()),
      status(no_sol),
      root_pd(std::make_unique<NodeData>(this)) {
    double start_time = 0.0;

    int val = program_header(argc, argv);
    val = parms.parse_cmd(argc, argv);

    if (dbg_lvl() > 1) {
        fmt::print("Debugging turned on\n");
    }

    /**
     * @brief Reading and preprocessing the data
     *
     */
    start_time = CCutil_zeit();
    fs::path _path{parms.jobfile};
    instance = Instance(_path, &parms);
    problem_read();
    preprocess_data();
    fmt::print("Reading and preprocessing of the data took {} seconds\n",
               CCutil_zeit() - start_time);
    heuristic_new();

    /**
     *@brief Finding heuristic solutions to the problem or start without
     *feasible solutions
     */
    if (parms.use_heuristic) {
        heuristic();
    } else {
        opt_sol = solution_alloc(intervals->len, nb_machines, nb_jobs, off);
        Solution* sol = opt_sol;
        val = construct_edd(sol);
        fmt::print("Solution Constructed with EDD heuristic:\n");
        solution_print(sol);
        solution_canonical_order(sol, intervals);
        fmt::print("Solution in canonical order: \n");
        solution_print(sol);
    }
    /**
     * @brief Build DD at the root node
     *
     */
    CCutil_start_timer(&(stat.tot_build_dd));
    switch (parms.pricing_solver) {
        case bdd_solver_simple:
            root_pd->solver = std::make_unique<PricerSolverBddSimple>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), H_max, nullptr, opt_sol->tw);
            break;
        case bdd_solver_cycle:
            root_pd->solver = std::make_unique<PricerSolverBddCycle>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), H_max, nullptr, opt_sol->tw);
            break;
        case zdd_solver_cycle:
            root_pd->solver = std::make_unique<PricerSolverZddCycle>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), global_upper_bound);
            break;
        case zdd_solver_simple:
            root_pd->solver = std::make_unique<PricerSolverSimple>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), opt_sol->tw);
            break;
        case bdd_solver_backward_simple:
            root_pd->solver = std::make_unique<PricerSolverBddBackwardSimple>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), H_max, nullptr, global_upper_bound);
            break;
        case bdd_solver_backward_cycle:
            root_pd->solver = std::make_unique<PricerSolverBddBackwardCycle>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), H_max, nullptr, global_upper_bound);
            ;
            break;
        case zdd_solver_backward_simple:
            root_pd->solver = std::make_unique<PricerSolverZddBackwardSimple>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), opt_sol->tw);
            break;
        case zdd_solver_backward_cycle:
            root_pd->solver = std::make_unique<PricerSolverZddBackwardCycle>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), opt_sol->tw);
        case dp_solver:
            root_pd->solver = std::make_unique<PricerSolverSimpleDp>(
                g_job_array, nb_machines, H_max, parms.pname.c_str(),
                opt_sol->tw);
            break;
        case ati_solver:
            root_pd->solver = std::make_unique<PricerSolverArcTimeDp>(
                g_job_array, nb_machines, H_max, parms.pname.c_str(),
                opt_sol->tw);
            break;
        case dp_bdd_solver:
            root_pd->solver = std::make_unique<PricerSolverSimpleDp>(
                g_job_array, nb_machines, H_max, parms.pname.c_str(),
                opt_sol->tw);
            break;
        default:
            root_pd->solver = std::make_unique<PricerSolverBddBackwardCycle>(
                g_job_array, nb_machines, root_pd->ordered_jobs,
                parms.pname.c_str(), H_max, nullptr, global_upper_bound);
            break;
    }
    CCutil_stop_timer(&(stat.tot_build_dd), 0);
    stat.first_size_graph = root_pd->solver->get_nb_vertices();

    /**
     * @brief Initial stabilization method
     *
     */
    auto* tmp_solver = root_pd->solver.get();
    switch (parms.stab_technique) {
        case stab_wentgnes:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationStat>(tmp_solver);
            break;
        case stab_dynamic:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationDynamic>(tmp_solver);
            break;
        case stab_hybrid:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationHybrid>(tmp_solver);
            break;
        case no_stab:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationBase>(tmp_solver);
            break;
        default:
            root_pd->solver_stab =
                std::make_unique<PricingStabilizationStat>(tmp_solver);
            break;
    }
    root_pd->stat = &(stat);
    root_pd->opt_sol = opt_sol;

    /**
     * @brief Initialization of the B&B tree
     *
     */
    tree = std::make_unique<BranchBoundTree>(std::move(root_pd), 0, 1);
    // tree->explore();
}

static void g_problem_summary_init(gpointer data, gpointer user_data) {
    Problem* prob = static_cast<Problem*>(user_data);
    Job*     j = static_cast<Job*>(data);

    prob->p_sum += j->processing_time;
    prob->pmax = std::max(prob->pmax, j->processing_time);
    prob->pmin = std::min(prob->pmin, j->processing_time);
    prob->dmax = std::max(prob->dmax, j->due_time);
    prob->dmin = std::min(prob->dmin, j->due_time);
}

void Problem::calculate_Hmax() {
    int       temp = 0;
    double    temp_dbl = 0.0;
    NodeData* pd = root_pd.get();

    temp = p_sum - pmax;
    temp_dbl = static_cast<double>(temp);
    temp_dbl = floor(temp_dbl / nb_machines);
    H_max = pd->H_max = static_cast<int>(temp_dbl) + pmax;
    H_min = pd->H_min = static_cast<int>(ceil(temp_dbl / nb_machines)) - pmax;

    GPtrArray* duration = g_ptr_array_copy(g_job_array, NULL, NULL);
    g_ptr_array_set_free_func(duration, NULL);
    g_ptr_array_sort(duration, g_compare_duration);

    int    m = 0;
    int    i = nb_jobs - 1;
    double tmp = p_sum;
    H_min = p_sum;
    do {
        Job* job = static_cast<Job*>(g_ptr_array_index(duration, i));
        tmp -= job->processing_time;
        m++;
        i--;

    } while (m < nb_machines - 1);

    H_min = pd->H_min = static_cast<int>(ceil(tmp / nb_machines));
    g_ptr_array_free(duration, TRUE);
    fmt::print(
        R"(H_max = {}, H_min = {},  pmax = {}, pmin = {}, p_sum = {}, off = {}
)",
        H_max, H_min, pmax, pmin, p_sum, off);
}

NodeData::~NodeData() {
    temporary_data_free();
    g_ptr_array_free(ordered_jobs, TRUE);
    // g_ptr_array_free(best_schedule, TRUE);
}

void Problem::solve() {
    tree->explore();
    if (parms.print) {
        print_to_csv();
    }
}

void Problem::heuristic_new() {
    auto                         ILS = nb_jobs / 2;
    auto                         IR = parms.nb_iterations_rvnd;
    boost::timer::auto_cpu_timer test;

    Sol best_sol(instance.nb_jobs, instance.nb_machines, instance.off);
    best_sol.construct_edd(instance.jobs);
    fmt::print("Solution Constructed with EDD heuristic:\n");
    best_sol.print_solution();
    best_sol.canonical_order(instance.intervals);
    fmt::print("Solution in canonical order: \n");
    best_sol.print_solution();

    /** Local Search */
    auto local = LocalSearchData(instance.nb_jobs, instance.nb_machines);
    local.RVND(best_sol);

    best_sol.canonical_order(instance.intervals);
    fmt::print("Solution after local search:\n");
    best_sol.print_solution();

    /** Generate Mersenne twister */
    std::array<std::mt19937::result_type, std::mt19937::state_size>
        random_data{};
    std::iota(std::begin(random_data), std::end(random_data), 4);
    std::seed_seq seeds(std::begin(random_data), std::end(random_data));
    std::mt19937  mt = std::mt19937(seeds);

    Sol sol(instance.nb_jobs, instance.nb_machines, instance.off);
    std::vector<PerturbationOperator> perturbation_moves = {
        PerturbationOperator(1, 2), PerturbationOperator(1, 2),
        PerturbationOperator(1, 3), PerturbationOperator(1, 3),
        PerturbationOperator(2, 2), PerturbationOperator(2, 2),
        PerturbationOperator(2, 3), PerturbationOperator(2, 3)};
    int iterations = 0;
    for (auto i = 0UL; i < IR; ++i) {
        Sol sol1{instance.nb_jobs, instance.nb_machines, instance.off};
        sol1.construct_random_shuffle(instance.jobs);
        sol = sol1;

        for (auto j = 0UL; j < ILS; ++j) {
            local.RVND(sol1);
            ++iterations;
            if (sol1.tw < sol.tw) {
                sol = sol1;
                j = 0UL;
            }
            std::uniform_int_distribution dist(0UL,
                                               perturbation_moves.size() - 1);

            auto nb_perturbation_moves = dist(mt);

            std::ranges::shuffle(perturbation_moves, mt);
            std::for_each(perturbation_moves.begin(),
                          perturbation_moves.begin() + nb_perturbation_moves,
                          [&sol1](const auto& it) { it(sol1); });
        }

        if (sol.tw < best_sol.tw) {
            best_sol = sol;
        }
    }

    fmt::print("Best new heuristics {}\n", local.get_iterations());
    fmt::print("{}\n", local.test_timer.cum_zeit);
    best_sol.print_solution();
}
