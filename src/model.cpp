#include <fmt/core.h>
#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <span>
#include <vector>
#include "gurobi_c.h"
#include "job.h"
#include "lp.h"
#include "scheduleset.h"
#include "util.h"
#include "wctprivate.h"

int grab_integer_solution(NodeData*                  pd,
                          std::vector<double> const& x,
                          double                     tolerance) {
    int          val = 0;
    double       incumbent = 0.0;
    int          tot_weighted = 0;
    ScheduleSet* tmp_schedule = nullptr;
    Job*         tmp_j = nullptr;

    val = lp_interface_objval(pd->RMP, &incumbent);
    val = lp_interface_get_nb_cols(pd->RMP, &pd->nb_cols);
    assert(pd->nb_cols - pd->id_pseudo_schedules == pd->localColPool.size());

    // g_ptr_array_free(pd->best_schedule, TRUE);
    pd->best_schedule.clear();
    // pd->best_schedule = g_ptr_array_new_with_free_func(g_scheduleset_free);

    for (guint i = 0; i < pd->localColPool.size(); ++i) {
        tmp_schedule = pd->localColPool[i].get();

        if (x[i + pd->id_pseudo_schedules] >= 1.0 - tolerance) {
            auto aux = std::make_shared<ScheduleSet>(*tmp_schedule);
            pd->best_schedule.emplace_back(aux);

            tot_weighted += aux->total_weighted_completion_time;

            if (pd->best_schedule.size() > pd->nb_machines) {
                fmt::print(
                    "ERROR: \"Integral\" solution turned out to be not "
                    "integral!\n");
                fflush(stdout);
                val = 1;
            }
        }
    }

    /** Write a check function */
    fmt::print("Intermediate schedule:\n");
    // print_schedule(pd->bestcolors, pd->nb_best);
    fmt::print("with total weight {}\n", tot_weighted);
    assert(fabs((double)tot_weighted - incumbent) <= EPS);

    if (tot_weighted < pd->upper_bound) {
        pd->upper_bound = tot_weighted;
        pd->best_objective = tot_weighted;
    }

    if (pd->upper_bound == pd->lower_bound) {
        pd->status = finished;
    }

    return val;
}

int NodeData::add_lhs_scheduleset_to_rmp(ScheduleSet* set) {
    int   val = 0;
    gsize aux = 0;
    // double* lhs_coeff_tmp =
    //     static_cast<double*>(&g_array_index(lhs_coeff, double, 0));

    // int* aux_int_array = static_cast<int*>(g_array_steal(id_row, &aux));
    // CC_IFFREE(aux_int_array, int);
    id_row.clear();
    coeff_row.clear();
    // double* aux_double_array =
    //     static_cast<double*>(g_array_steal(coeff_row, &aux));
    // CC_IFFREE(aux_double_array, double);

    for (int j = 0; j < nb_rows; j++) {
        if (std::fabs(lhs_coeff[j]) > EPS_BOUND) {
            id_row.emplace_back(j);
            coeff_row.emplace_back(lhs_coeff[j]);
        }
    }

    // int*    id_constraint = static_cast<int*>(&g_array_index(id_row, int,
    // 0));
    // double* coeff_constraint =
    //     static_cast<double*>(&g_array_index(coeff_row, double, 0));
    int len = id_row.size();

    val = lp_interface_get_nb_cols(RMP, &(nb_cols));
    set->id = nb_cols - id_pseudo_schedules;
    auto cost = static_cast<double>(set->total_weighted_completion_time);
    val = lp_interface_addcol(RMP, len, id_row.data(), coeff_row.data(), cost,
                              0.0, GRB_INFINITY, lp_interface_CONT, NULL);

    return val;
}

int NodeData::add_scheduleset_to_rmp(ScheduleSet* set) {
    int        val = 0;
    int        row_ind = 0;
    int        var_ind = 0;
    double     cval = 0.0;
    double     cost = static_cast<double>(set->total_weighted_completion_time);
    GPtrArray* members = set->job_list;
    wctlp*     lp = RMP;
    Job*       job = nullptr;

    val = lp_interface_get_nb_cols(lp, &(nb_cols));
    var_ind = nb_cols;
    set->id = nb_cols - id_pseudo_schedules;
    val = lp_interface_addcol(lp, 0, nullptr, nullptr, cost, 0.0, GRB_INFINITY,
                              lp_interface_CONT, nullptr);

    for (unsigned i = 0; i < members->len; ++i) {
        job = static_cast<Job*>(g_ptr_array_index(members, i));
        row_ind = job->job;
        val = lp_interface_getcoeff(lp, &row_ind, &var_ind, &cval);
        cval += 1.0;
        val = lp_interface_chgcoeff(lp, 1, &row_ind, &var_ind, &cval);
    }

    row_ind = nb_jobs;
    cval = -1.0;
    val = lp_interface_chgcoeff(lp, 1, &row_ind, &var_ind, &cval);

    return val;
}

int NodeData::build_rmp() {
    int                 val = 0;
    double*             dbl_values{};
    int*                int_values{};
    std::vector<int>    start(nb_jobs, 0);
    std::vector<double> rhs_tmp(nb_jobs, 1.0);
    std::vector<char>   sense(nb_jobs, GRB_GREATER_EQUAL);
    val = lp_interface_init(&(RMP), NULL);

    /**
     * add assignment constraints
     */
    lp_interface_get_nb_rows(RMP, &(id_assignment_constraint));
    lp_interface_addrows(RMP, nb_jobs, 0, start.data(), NULL, NULL,
                         sense.data(), rhs_tmp.data(), NULL);

    /**
     * add number of machines constraint (convexification)
     */
    lp_interface_get_nb_rows(RMP, &(id_convex_constraint));
    val = lp_interface_addrow(RMP, 0, nullptr, nullptr,
                              lp_interface_GREATER_EQUAL,
                              -static_cast<double>(nb_machines), nullptr);
    // CCcheck_val_2(val, "Failed to add convexification constraint");
    lp_interface_get_nb_rows(RMP, &(id_valid_cuts));
    nb_rows = id_valid_cuts;

    /**
     * construct artificial variables in RMP
     */
    lp_interface_get_nb_cols(RMP, &(id_art_var_assignment));
    id_art_var_convex = nb_jobs;
    id_art_var_cuts = nb_jobs + 1;
    id_next_var_cuts = id_art_var_cuts;
    int nb_vars = nb_jobs + 1 + max_nb_cuts;
    id_pseudo_schedules = nb_vars;

    std::vector<double> lb(nb_vars, 0.0);
    std::vector<double> ub(nb_vars, GRB_INFINITY);
    std::vector<double> obj(nb_vars, 100.0 * (opt_sol->tw + 1));
    std::vector<char>   vtype(nb_vars, GRB_CONTINUOUS);
    std::vector<int>    start_vars(nb_vars + 1, nb_jobs + 1);

    start_vars[nb_vars] = nb_jobs + 1;

    std::vector<int> rows_ind(nb_jobs + 1);
    std::iota(rows_ind.begin(), rows_ind.end(), 0);
    std::vector<double> coeff_vals(nb_jobs + 1, 1.0);
    // std::iota(start_vars.begin(), start_vars.begin() + nb_jobs + 1, 0);

    for (int j = 0; j < nb_jobs + 1; j++) {
        //     rows_ind[j] = j;
        start_vars[j] = j;
        //     coeff_vals[j] = 1.0;
    }

    val = lp_interface_addcols(RMP, nb_vars, nb_jobs + 1, start_vars.data(),
                               rows_ind.data(), coeff_vals.data(), obj.data(),
                               lb.data(), ub.data(), vtype.data(), nullptr);
    // CCcheck_val(val, "failed construct cols");
    val = lp_interface_get_nb_cols(RMP, &(id_pseudo_schedules));

    /** add columns from localColPool */
    // add_artificial_var_to_rmp(pd);
    // g_ptr_array_foreach(localColPool, g_add_col_to_lp, this);

    std::for_each(localColPool.begin(), localColPool.end(),
                  [&](auto& it) { add_scheduleset_to_rmp(it.get()); });

    /**
     * Some aux variables for column generation
     */

    pi.resize(nb_rows, 0.0);
    slack.resize(nb_rows, 0.0);
    rhs.resize(nb_rows, 0.0);
    val = lp_interface_get_rhs(RMP, rhs.data());
    lhs_coeff.resize(nb_rows, 0.0);
    id_row.reserve(nb_rows);
    coeff_row.reserve(nb_rows);

CLEAN:

    if (val) {
        lp_interface_free(&(RMP));
    }
    return val;
}

// int NodeData::get_solution_lp_lowerbound() {
//     int  val = 0;
//     Job* tmp_j = nullptr;

//     val = lp_interface_get_nb_cols(RMP, &nb_cols);
//     lambda.resize(nb_cols, 0.0);
//     lp_interface_x(RMP, lambda.data(), 0);

//     for (int i = 0; i < nb_cols; ++i) {
//         ScheduleSet* tmp = localColPool[i].get();
//         if (lambda[i] > EPS) {
//             fmt::print("{}: ", lambda[i]);
//             g_ptr_array_foreach(tmp->job_list, g_print_machine, NULL);
//             fmt::print("\n");
//             for (unsigned j = 0; j < tmp->job_list->len; ++j) {
//                 tmp_j = (Job*)g_ptr_array_index(tmp->job_list, j);
//                 // printf("%d (%d) ", tmp->num[tmp_j->job],
//                 //    tmp_j->processing_time);
//             }
//             fmt::print("\n");
//         }
//     }

// CLEAN:
//     return val;
// }
