#include "BranchNode.hpp"
#include <fmt/core.h>
#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <numeric>
#include <range/v3/action/sort.hpp>
#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/count.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/numeric/iota.hpp>
#include <range/v3/view/counted.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/zip.hpp>
#include <span>
#include <utility>
#include <vector>
#include "Parms.h"
#include "PricerSolverBase.hpp"
#include "Statistics.h"
#include "branch-and-bound/btree.h"
#include "util.h"
#include "wctprivate.h"

BranchNodeBase::BranchNodeBase(std::unique_ptr<NodeData> _pd, bool _isRoot)
    : State(_isRoot),
      pd(std::move(_pd)) {
    if (_isRoot) {
        pd->build_rmp();
        pd->solve_relaxation();
        pd->stat.start_resume_timer(Statistics::lb_root_timer);
        pd->compute_lower_bound();
        pd->stat.suspend_timer(Statistics::lb_root_timer);
        set_lb(pd->lower_bound);
        set_obj_value(pd->LP_lower_bound);
    }
}

void BranchNodeBase::branch(BTree* bt) {
    auto*       solver = pd->solver.get();
    auto        strong_branching = false;
    const auto& instance = pd->instance;
    const auto& parms = pd->parms;

    if (!strong_branching && dbg_lvl() > 0) {
        fmt::print("\nDOING STRONG BRANCHING...\n\n");
    }

    if (bt->getGlobalUB() < pd->upper_bound) {
        pd->upper_bound = static_cast<int>(bt->getGlobalUB());
        solver->UB = bt->getGlobalUB();
    }

    // calculate the fraction of each job finishing at each time in the
    // relaxation solution
    std::vector<std::vector<double>> x_job_time(instance.nb_jobs);
    for (auto& it : x_job_time) {
        it.resize(pd->instance.H_max + 1, 0.0);
    }

    solver->calculate_job_time(&x_job_time);

    // initialize the middle times and scores used for branching
    std::vector<BranchCand> best_cand{};

    for (auto&& [x_j, job] : ranges::views::zip(x_job_time, instance.jobs)) {
        auto prev = -1;
        auto accum = 0.0;
        auto dist_zero = 0.0;
        auto frac_sol = false;
        auto middle_time = -1;
        auto branch_score = 0.0;
        auto C_count = 0;
        auto is_positive{[&C_count](const auto& tmp) {
            if (std::get<1>(tmp) > EPS) {
                C_count++;
                return true;
            }
            return false;
        }};

        auto C_range{x_j | ranges::views::enumerate |
                     ranges::views::filter(is_positive)};

        for (auto&& [t, x] : C_range) {
            accum += x;
            if ((accum >= (1.0 - TargetBrTimeValue)) && (prev != -1) &&
                (middle_time == -1)) {
                middle_time = (t + job->processing_time + prev) / 2;
                branch_score =
                    double(job->weighted_tardiness(middle_time)) * accum -
                    dist_zero;
            }

            if (middle_time != -1) {
                branch_score +=
                    double(job->weighted_tardiness(t + job->processing_time) -
                           job->weighted_tardiness(middle_time)) *
                    x;
                frac_sol = true;
            }

            dist_zero +=
                double(job->weighted_tardiness(t + job->processing_time)) * x;

            prev = t + job->processing_time;
        }

        if (frac_sol) {
            best_cand.emplace_back(branch_score, job->job, middle_time);
        }
    }

    auto best_min_gain = 0.0;
    auto best_job = -1;
    auto best_time = 0;

    std::unique_ptr<BranchNodeBase> best_right = nullptr;
    std::unique_ptr<BranchNodeBase> best_left = nullptr;
    auto nb_cand = std::min(NumStrBrCandidates, best_cand.size());
    best_cand |= ranges::actions::sort(std::greater{}, &BranchCand::score);

    for (auto& it : best_cand | ranges::views::take(nb_cand)) {
        auto i = static_cast<size_t>(it.job);

        auto  left_gain = 0.0;
        auto  right_gain = 0.0;
        auto* job = pd->instance.jobs[i].get();

        if (strong_branching) {
            for (auto&& [t, x] : x_job_time[i] | ranges::views::enumerate) {
                if (t + job->processing_time <= it.t) {
                    left_gain += x;
                } else {
                    right_gain += x;
                }
            }

            auto min_gain = std::min(right_gain, left_gain);

            if (min_gain > best_min_gain) {
                best_min_gain = min_gain;
                best_job = it.job;
                best_time = it.t;
            }
        } else {
            auto data_nodes{pd->create_child_nodes(it.job, it.t)};
            std::array<std::unique_ptr<BranchNodeBase>, 2> child_nodes;
            std::array<double, 2>                          scores{};
            std::array<bool, 2>                            fathom{};
            auto                                           left = true;

            for (auto&& [data, node, score, f] :
                 ranges::views::zip(data_nodes, child_nodes, scores, fathom)) {
                node = std::make_unique<BranchNodeBase>(std::move(data));
                node->compute_bounds(bt);

                auto cost = node->pd->LP_lower_bound;
                if (dbg_lvl() > 0) {
                    fmt::print(
                        "STRONG BRANCHING {} PROBE: j = {}, t = {},"
                        " DWM LB = {:9.2f} in iterations {} \n\n",
                        left ? "LEFT" : "RIGHT", it.job, it.t,
                        cost + pd->instance.off, node->pd->iterations);
                }
                if (cost >= pd->opt_sol.tw - 1.0 + IntegerTolerance ||
                    node->get_data_ptr()->solver->get_is_integer_solution()) {
                    f = true;
                }
                score =
                    (parms.scoring_parameter == weighted_sum_scoring_parameter)
                        ? cost / pd->LP_lower_bound
                        : std::abs(cost - pd->LP_lower_bound);
                left = false;
            }

            auto best_score = parms.scoring_function(scores[0], scores[1]);

            if (best_score > best_min_gain ||
                ranges::any_of(fathom, ranges::identity{})) {
                best_min_gain = best_score;
                best_right = std::move(child_nodes[1]);
                best_left = std::move(child_nodes[0]);
            }

            if (ranges::any_of(fathom, std::identity{})) {
                break;
            }
        }
    }

    if (best_cand.empty()) {
        fmt::print(stderr, "ERROR: no branching found!\n");
        for (auto&& [j, job] : instance.jobs | ranges::views::enumerate) {
            fmt::print(stderr, "j={}:", j);
            for (auto&& [t, x] :
                 x_job_time[j] | ranges::views::enumerate |
                     ranges::views::filter(
                         [&](const auto& tmp) { return (tmp.second > EPS); })) {
                fmt::print(stderr, " ({},{})", t + job->processing_time, x);
            }
            fmt::print(stderr, "\n");
        }

        exit(-1);
    }

    /** Process the branching nodes insert them in the tree */
    if (strong_branching) {
        auto  left = pd->clone();
        auto* left_solver = left->solver.get();
        auto  left_node_branch =
            std::make_unique<BranchNodeBase>(std::move(left));
        left_solver->split_job_time(best_job, best_time, false);
        left_node_branch->pd->branch_job = best_job;
        left_node_branch->pd->completiontime = best_time;
        left_node_branch->pd->less = 0;
        bt->process_state(std::move(left_node_branch));

        auto  right = pd->clone();
        auto* right_solver = right->solver.get();
        auto  right_node_branch =
            std::make_unique<BranchNodeBase>(std::move(right));
        right_solver->split_job_time(best_job, best_time, true);
        right_node_branch->pd->branch_job = best_job;
        right_node_branch->pd->completiontime = best_time;
        right_node_branch->pd->less = 1;
        bt->process_state(std::move(right_node_branch));
    } else {
        bt->set_state_bounds_computed(true);
        bt->process_state(std::move(best_right));
        bt->process_state(std::move(best_left));
        bt->set_state_bounds_computed(false);
    }

    bt->update_global_lb();

    if (dbg_lvl() > 1) {
        fmt::print("Branching choice: j = {}, t = {}, best_gain = {}\n",
                   best_job, best_time, best_min_gain + pd->instance.off);
    }
}

void BranchNodeBase::compute_bounds(BTree* bt) {
    pd->build_rmp();
    pd->solve_relaxation();
    pd->compute_lower_bound();
    set_lb(pd->lower_bound);
    if (pd->solver->get_is_integer_solution()) {
        set_obj_value(pd->lower_bound);
    }
}

void BranchNodeBase::assess_dominance([[maybe_unused]] State* otherState) {}

bool BranchNodeBase::is_terminal_state() {
    auto* solver = pd->solver.get();
    return solver->get_is_integer_solution();
}

void BranchNodeBase::apply_final_pruning_tests(BTree* bt) {}

void BranchNodeBase::update_data(double upper_bound) {
    // auto* statistics = pd->stat;
    pd->stat.global_upper_bound = static_cast<int>(upper_bound);
}

void BranchNodeBase::print(const BTree* bt) const {
    auto* solver = pd->solver.get();
    if (get_is_root_node()) {
        fmt::print("{0:^10}|{1:^30}|{2:^30}|{3:^10}|{4:^10}|\n", "Nodes",
                   "Current Node", "Objective Bounds", "Branch", "Work");
        fmt::print(
            R"({0:^5}{1:^5}|{2:^10}{3:^10}{10:^10}|{4:>10}{5:>10}{6:>10}|{7:>5}{8:>5}|{9:>5}{11:>5}
)",
            "Expl", "UnEx", "Obj", "Depth", "Primal", "Dual", "Gap", "Job",
            "Time", "Time", "Size", "Iter");
    }
    fmt::print(
        R"({0:>5}{1:>5}|{2:10.2f}{3:>10}{4:>10}|{5:10.2f}{6:10.2f}{7:10.2f}|{8:>5}{9:>5}|{10:>5}{11:>5}|
)",
        bt->get_nb_nodes_explored(), bt->get_nb_nodes(),
        pd->LP_lower_bound + pd->instance.off, pd->depth,
        solver->get_nb_vertices(), bt->getGlobalUB() + pd->instance.off,
        bt->getGlobalLB() + pd->instance.off, 0.0, pd->branch_job,
        pd->completiontime, bt->get_run_time_start(), pd->iterations);
}

BranchCand::BranchCand(int _job, int _t, const NodeData* parent)
    : job(_job),
      t(_t) {
    //   left(std::move(std::make_unique<BranchNodeBase>(parrent->clone()))),
    //   right(std::mo(std::make_unique<BranchNodeBase>(parrent->clone()))) {
    // auto* left_data_ptr = left->get_data_ptr();
    // auto* right_data_ptr = right->get_data_ptr();

    // left_data_ptr->solver->split_job_time(job, t, true);
    // left_data_ptr->build_rmp();
    // left_data_ptr->solve_relaxation();
    // left_data_ptr->estimate_lower_bound(10);

    // right_data_ptr->solver->split_job_time(job, t, false);
    // right_data_ptr->build_rmp();
    // right_data_ptr->solve_relaxation();
    // right_data_ptr->estimate_lower_bound(10);

    // score = std::min(right_data_ptr->LP_lower_bound -
    // parrent->LP_lower_bound,
    //                  left_data_ptr->LP_lower_bound -
    //                  parrent->LP_lower_bound);
}

BranchCand::BranchCand(double _score, int _job, int _t)
    : score(_score),
      job(_job),
      t(_t) {}