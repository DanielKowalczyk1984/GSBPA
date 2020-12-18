#include "BranchNode.hpp"
#include <fmt/core.h>
#include <cstdio>
#include "PricerSolverBase.hpp"
#include "branch-and-bound/btree.h"
// #include "branch-and-boundwrapper.h"
#include "solver.h"
#include "wct.h"
#include "wctprivate.h"

BranchNodeBase::BranchNodeBase(NodeData* _pd, bool _isRoot)
    : State(_isRoot),
      pd(_pd) {
    setDepth(pd->depth);
}

void BranchNodeBase::branch(BTree* bt) {
    auto solver = pd->solver;
    auto nb_jobs = pd->nb_jobs;
    auto strong_branching =
        ((pd->opt_sol->tw - pd->LP_lower_bound) < (1.0 + IntegerTolerance));

    if (strong_branching) {
        fmt::print("\nDOING STRONG BRANCHING...\n\n");
    }

    auto fathom_left = false;
    auto fathom_right = false;

    // calculate the fraction of each job finishing at each time in the
    // relaxation solution
    std::vector<std::vector<double>> x_job_time(nb_jobs);
    for (auto i = 0; i < nb_jobs; i++) {
        x_job_time[i].resize(pd->H_max + 1, 0.0);
    }

    solver->calculate_job_time(x_job_time);

    // initialize the order for evaluating the branching jobs
    std::vector<int> ord(nb_jobs);
    for (int k = 0; k < nb_jobs; k++) {
        ord[k] = k;
    }

    // initialize the middle times and scores used for branching
    std::vector<int>    middle_time(nb_jobs, -1);
    std::vector<double> branch_scores(nb_jobs, 0.0);

    for (auto k = 0; k < nb_jobs; k++) {
        auto i = ord[k];
        auto prev = -1;
        auto accum = 0.0;
        auto dist_zero = 0.0;
        auto job = (Job*)g_ptr_array_index(solver->jobs, i);
        for (auto t = 0; t < pd->H_max + 1; t++) {
            accum += x_job_time[i][t];
            if ((accum >= (1.0 - TargetBrTimeValue)) &&
                (x_job_time[i][t] > 1e-3) && (prev != -1) &&
                (middle_time[i] == -1)) {
                middle_time[i] = (t + job->processing_time + prev + 1) / 2;
                branch_scores[i] = double(middle_time[i]) * accum - dist_zero;
            }
            if (middle_time[i] != -1)
                branch_scores[i] +=
                    double(t + job->processing_time - middle_time[i] + 1) *
                    x_job_time[i][t];
            dist_zero += double(t + job->processing_time) * x_job_time[i][t];
            if (x_job_time[i][t] > 1e-3)
                prev = t;
        }
    }

    // choose the candidates for strong branching
    std::vector<double> best_scores(NumStrBrCandidates, 1e-3);
    std::vector<int>    best_jobs(NumStrBrCandidates, -1);
    for (auto i = 0; i < nb_jobs; i++) {
        // update the array of best minimum estimated gains
        auto worse = 0;
        for (auto k = 0; k < NumStrBrCandidates; k++) {
            if (best_scores[k] < best_scores[worse])
                worse = k;
        }
        if (best_scores[worse] < branch_scores[i]) {
            best_scores[worse] = branch_scores[i];
            best_jobs[worse] = i;
        }
    }

    auto            best_min_gain = 0.0;
    auto            best_job = -1;
    auto            best_time = 0;
    BranchNodeBase* best_right = nullptr;
    BranchNodeBase* best_left = nullptr;
    for (auto k = 0; k < NumStrBrCandidates; k++) {
        auto i = best_jobs[k];
        if (i == -1) {
            continue;
        }

        auto left_gain = 0.0;
        auto right_gain = 0.0;
        for (auto t = 0; t < pd->H_max; t++) {
            if (t < middle_time[i]) {
                left_gain += x_job_time[i][t];
            } else {
                right_gain += x_job_time[i][t];
            }
        }

        if (strong_branching) {
            auto left = new_node_data(pd);
            auto left_solver = left->solver;
            auto left_node_branch = new BranchNodeBase(left);
            left->solver->split_job_time(i, middle_time[i], false);
            left_node_branch->computeBounds(bt);

            auto approx = left_gain;
            left_gain = left->LP_lower_bound;
            fmt::print(
                "STRONG BRANCHING LEFT PROBE: j = {}, t = {},"
                " DWM LB = {:9.2f} ({})\n\n",
                i, middle_time[i], left_gain, approx);
            if (left_gain >= pd->opt_sol->tw - 1.0 + IntegerTolerance) {
                fathom_left = true;
            }

            // build the right node and solve its root LP only

            auto right = new_node_data(pd);
            auto right_solver = right->solver;
            auto right_node_branch = new BranchNodeBase(right);

            right_solver->split_job_time(i, middle_time[i], true);
            right_node_branch->computeBounds(bt);

            approx = right_gain;
            right_gain = right->LP_lower_bound;
            fmt::print(
                "STRONG BRANCHING RIGHT PROBE: j = {}, t= {},"
                " DWM LB = {:9.2f} ({})\n\n",
                i, middle_time[i], right_gain, approx);
            if (right_gain >= pd->opt_sol->tw - 1.0 + IntegerTolerance) {
                fathom_right = true;
            }

            // stop if fathomed in any side
            if (fathom_left || fathom_right) {
                /** Process everything */
                bt->setStateComputesBounds(true);
                bt->processState(left_node_branch);
                bt->processState(right_node_branch);
                bt->setStateComputesBounds(false);
                fmt::print("Number of Nodes in B&B tree = {}",
                           bt->get_nb_nodes());
                return;
            }

            // update the branching choice
            auto min_gain = std::min(right_gain, left_gain);

            if (min_gain > best_min_gain) {
                best_min_gain = min_gain;
                best_job = i;
                best_time = middle_time[i];
                if (best_right) {
                    delete best_right;
                }
                best_right = right_node_branch;

                if (best_left) {
                    delete best_left;
                }
                best_left = left_node_branch;
                right_node_branch = nullptr;
                left_node_branch = nullptr;
            }

            if (left_node_branch) {
                delete left_node_branch;
            }
            if (right_node_branch) {
                delete right_node_branch;
            }
        } else {
            auto min_gain = std::min(right_gain, left_gain);

            if (min_gain > best_min_gain) {
                best_min_gain = min_gain;
                best_job = i;
                best_time = middle_time[i];
            }
        }
    }

    if (best_job == -1) {
        fprintf(stderr, "ERROR: no branching found!\n");
        for (auto k = 0; k < nb_jobs; k++) {
            auto j = ord[k];
            fprintf(stderr, "j=%d:", j);
            for (int t = 0; t < pd->H_max; t++)
                if (x_job_time[j][t] > 1e-12)
                    fprintf(stderr, " (%d,%lg)", t, x_job_time[j][t]);
            fprintf(stderr, "\n");
        }

        exit(-1);
    }

    /** Process the branching nodes insert them in the tree */
    if (strong_branching) {
        bt->setStateComputesBounds(true);
        bt->processState(best_left);
        bt->processState(best_right);
        bt->setStateComputesBounds(false);
    } else {
        auto left = new_node_data(pd);
        auto left_solver = left->solver;
        auto left_node_branch = new BranchNodeBase(left);
        left->solver->split_job_time(best_job, best_time, false);
        bt->processState(left_node_branch);

        auto right = new_node_data(pd);
        auto right_solver = right->solver;
        auto right_node_branch = new BranchNodeBase(right);
        right_solver->split_job_time(best_job, best_time, true);
        bt->processState(right_node_branch);
    }
    fmt::print("Number of Nodes in B&B tree = {}", bt->get_nb_nodes());
    getchar();

    fmt::print("Branching choice: j= {}, t= {}\n", best_job, best_time);
}

void BranchNodeBase::computeBounds(BTree* bt) {
    build_rmp(pd);
    solve_relaxation(pd);
    compute_lower_bound(pd);
    lowerBound = pd->LP_lower_bound;
    objValue = pd->opt_sol->tw;
}

void BranchNodeBase::assessDominance(State* otherState) {}

bool BranchNodeBase::isTerminalState() {
    return false;
}

void BranchNodeBase::applyFinalPruningTests(BTree* bt) {}

extern "C" {

BranchNodeBase* new_branch_node(int _isRoot, NodeData* data) {
    return new BranchNodeBase(data, _isRoot);
};

void delete_branch_node(BranchNodeBase* node) {
    if (node) {
        delete node;
    }
}

size_t call_getDepth(BranchNodeBase* state) {
    return state->getDepth();
};
int call_getDomClassID(BranchNodeBase* state) {
    return state->getDomClassID();
};
double call_getObjValue(BranchNodeBase* state) {
    return state->getObjValue();
};
double call_getLB(BranchNodeBase* state) {
    return state->getLB();
};
double call_getUB(BranchNodeBase* state) {
    return state->getUB();
};
int call_getID(BranchNodeBase* state) {
    return state->getID();
}
int call_getParentID(BranchNodeBase* state) {
    return state->getParentID();
}
void call_setID(BranchNodeBase* state, int i) {
    state->setID(i);
}
bool call_isDominated(BranchNodeBase* state) {
    return state->isDominated();
};
bool call_wasProcessed(BranchNodeBase* state) {
    return state->wasProcessed();
};
}