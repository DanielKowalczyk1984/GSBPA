#include <fmt/chrono.h>
#include <fmt/core.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include "BranchBoundTree.hpp"
#include "Statistics.h"
#include "wctprivate.h"

void Problem::to_csv() {
    using ptr_file = std::unique_ptr<std::FILE, std::function<int(FILE*)>>;

    ptr_file    file{};
    std::time_t result = std::time(nullptr);
    const auto  current_t = std::localtime(&result);

    std::string file_name =
        fmt::format("CG_overall_{:%y_%m_%d}.csv", *current_t);
    stat.real_time_total = getRealTime() - stat.real_time_total;
    CCutil_stop_timer(&(stat.tot_cputime), 0);

    if (access(file_name.c_str(), F_OK) != -1) {
        file = ptr_file(std::fopen(file_name.c_str(), "a"), &fclose);
    } else {
        file = ptr_file(std::fopen(file_name.c_str(), "w"), &fclose);
        if (file == nullptr) {
            throw ProblemException(
                fmt::format("We could not open file {} in {} at {}", file_name,
                            __FILE__, __LINE__)
                    .c_str());
        }
        fmt::print(
            file.get(),
            R"({},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}
)",
            "NameInstance", "n", "m", "tot_real_time", "tot_cputime", "tot_bb",
            "tot_lb", "tot_lb_root", "tot_heuristic", "tot_build_dd",
            "tot_pricing", stat.tot_reduce_cost_fixing.name, "rel_error",
            "global_lower_bound", "global_upper_bound", "first_rel_error",
            "global_upper_bound_root", "global_lowerbound_root",
            "nb_generated_col", "nb_generated_col_root", "nb_nodes_explored",
            "date", "nb_iterations_rvnd", "stabilization", "alpha",
            "pricing_solver", "first_size_graph", "size_after_reduced_cost",
            "mip_nb_vars", "mip_nb_constr", "mip_obj_bound", "mip_obj_bound_lp",
            "mip_rel_gap", "mip_run_time", "mip_status", "mip_nb_iter_simplex",
            "mip_nb_nodes");
    }

    // for (int i = MIP_Attr_Run_Time; i <= MIP_Attr_Nb_Nodes &&
    // parms.mip_solver;
    //      i++) {
    //     pd->get_mip_statistics(static_cast<MIP_Attr>(i));
    // }

    fmt::print(
        file.get(),
        R"({},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{:%y/%m/%d},{},{},{},{},{},{},{},{},{},{},{},{},{},{}
)",
        stat.pname, instance.nb_jobs, instance.nb_machines,
        stat.real_time_total, stat.total_timer(Statistics::cputime_timer),
        stat.total_timer(Statistics::bb_timer),
        stat.total_timer(Statistics::lb_timer),
        stat.total_timer(Statistics::lb_root_timer),
        stat.total_timer(Statistics::heuristic_timer),
        stat.total_timer(Statistics::build_dd_timer),
        stat.total_timer(Statistics::pricing_timer),
        stat.total_timer(Statistics::reduced_cost_fixing_timer), stat.rel_error,
        stat.global_lower_bound, stat.global_upper_bound, stat.root_rel_error,
        stat.root_upper_bound, stat.root_lower_bound, stat.nb_generated_col,
        stat.nb_generated_col_root, tree->get_nb_nodes_explored(), *current_t,
        parms.nb_iterations_rvnd, parms.stab_technique, parms.alpha,
        parms.pricing_solver, stat.first_size_graph,
        stat.size_graph_after_reduced_cost_fixing, stat.mip_nb_vars,
        stat.mip_nb_constr, stat.mip_obj_bound, stat.mip_obj_bound_lp,
        stat.mip_rel_gap, stat.mip_run_time, stat.mip_status,
        stat.mip_nb_iter_simplex, stat.mip_nb_nodes);
}

int Problem::to_screen() {
    int val = 0;

    switch (status) {
        case no_sol:
            fmt::print(
                "We didn't decide if this instance is feasible or "
                "infeasible\n");
            break;

        case feasible:
        case lp_feasible:
        case meta_heuristic:
            fmt::print(
                "A suboptimal schedule with relative error {} is found.\n",
                static_cast<double>(stat.global_upper_bound -
                                    stat.global_lower_bound) /
                    (stat.global_lower_bound));
            break;

        case optimal:
            fmt::print("The optimal schedule is found.\n");
            break;
    }

    fmt::print(
        "Compute_schedule took {} seconds(tot_scatter_search {}, "
        "tot_branch_and_bound {}, tot_lb_lp_root {}, tot_lb_lp {}, tot_lb "
        "{}, "
        "tot_pricing {}, tot_build_dd {}) and {} seconds in real time\n",
        stat.tot_cputime.cum_zeit, stat.tot_heuristic.cum_zeit,
        stat.tot_branch_and_bound.cum_zeit, stat.tot_lb_root.cum_zeit,
        stat.tot_lb.cum_zeit, stat.tot_lb.cum_zeit, stat.tot_pricing.cum_zeit,
        stat.tot_build_dd.cum_zeit, stat.real_time_total);
    return val;
}
