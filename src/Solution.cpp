#include "Solution.hpp"
#include <bits/ranges_algo.h>
#include <fmt/core.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <random>
#include <range/v3/action/erase.hpp>
#include <range/v3/action/insert.hpp>
#include <range/v3/all.hpp>
#include <vector>
#include "Instance.h"
#include "Interval.h"
#include "Job.h"

void Sol::construct_edd(std::vector<std::shared_ptr<Job>>& v) {
    auto cmp_jobs_edd = [](const auto lhs, const auto rhs) -> bool {
        return lhs->due_time < rhs->due_time;
    };
    std::vector<Job*> tmp{};

    std::ranges::transform(v, std::back_inserter(tmp),
                           [](const auto& tmp_j) { return tmp_j.get(); });
    std::ranges::sort(tmp, cmp_jobs_edd);
    tw = 0;

    // std::make_heap(machines.begin(), machines.end(),
    // cmp_machines_completion);
    ranges::make_heap(machines, ranges::greater{}, &Machine::completion_time);

    for (auto& it : tmp) {
        add_job_front_machine(it);
        ranges::push_heap(machines, ranges::greater{},
                          &Machine::completion_time);
    }
}

void Sol::construct_spt(const std::vector<std::shared_ptr<Job>>& v) {
    auto cmp_jobs_spt = [](const auto x, const auto y) -> bool {
        // if (x->processing_time > y->processing_time) {
        //     return false;
        // } else if (x->due_time > y->due_time) {
        //     return false;
        // } else if (x->weight > y->weight) {
        //     return false;
        // } else if (x->job > y->job) {
        //     return false;
        // } else {
        //     return true;
        // }

        return std::tie(x->processing_time, x->due_time, x->weight, x->job) <
               std::tie(y->processing_time, y->due_time, y->weight, y->job);
    };

    tw = 0;
    auto tmp =
        v | ranges::views::transform([](const auto ptr) { return ptr.get(); }) |
        ranges::to<std::vector>() | ranges::actions::sort(cmp_jobs_spt);

    for (auto& it : tmp) {
        add_job_front_machine(it);
        ranges::push_heap(machines, ranges::greater{},
                          &Machine::completion_time);
    }
}

void Sol::construct_random_fisher_yates(
    const std::vector<std::shared_ptr<Job>>& v) {
    auto tmp =
        v | ranges::views::transform([](const auto ptr) { return ptr.get(); }) |
        ranges::to<std::vector>();

    tw = 0;
    for (auto i = 0UL; i < tmp.size() - 1; i++) {
        auto j = i + rand() % (tmp.size() - i);
        std::swap(tmp[i], tmp[j]);
    }

    for (auto& it : tmp) {
        add_job_front_machine(it);
        ranges::push_heap(machines, ranges::greater{},
                          &Machine::completion_time);
    }
}

void Sol::construct_random_shuffle(const std::vector<std::shared_ptr<Job>>& v) {
    std::random_device         rd;
    std::default_random_engine rng(rd());
    tw = 0;
    auto tmp =
        v | ranges::views::transform([](const auto ptr) { return ptr.get(); }) |
        ranges::to<std::vector>();

    tmp |= ranges::actions::shuffle(rng);

    for (auto& it : tmp) {
        add_job_front_machine(it);
        ranges::push_heap(machines, ranges::greater{},
                          &Machine::completion_time);
    }
}

void Sol::canonical_order(const VecIntervalPtr& intervals) {
    tw = 0;
    for (auto& m : machines) {
        std::vector<VecJobRawPtr> Q(intervals.size(), VecJobRawPtr());
        std::vector<VecJobRawPtr> Q_in(intervals.size(), VecJobRawPtr());

        auto it_I = 0UL;
        for (auto& job_it : m.job_list) {
            auto job = job_it->job;
            auto C = c[job];
            while (!(C > intervals[it_I]->a && C <= intervals[it_I]->b)) {
                it_I++;
                if (it_I == intervals.size()) {
                    it_I--;
                    break;
                }
            }

            u[job] = it_I;
            Q[it_I].push_back(job_it);
            if (c[job] - job_it->processing_time > intervals[it_I]->a ||
                (it_I == 0U && c[job] - job_it->processing_time == 0)) {
                Q_in[it_I].push_back(job_it);
            }
        }

        auto u_it = u[m.job_list.back()->job];
        while (u_it + 1 != 0UL) {
            auto& I = intervals[u_it];
            auto& Q_tmp = Q[u_it];
            auto& Q_in_tmp = Q_in[u_it];
            if (!Q_in_tmp.empty()) {
                if (Q_in_tmp.size() + 1 == Q_tmp.size()) {
                    auto C = c[Q_in_tmp.front()->job] -
                             Q_in_tmp.front()->processing_time;
                    auto cmp = compare_edd(I->a, I->b);
                    std::ranges::sort(Q_in_tmp, cmp);

                    auto j = 0UL;
                    for (auto& it : Q_in_tmp) {
                        Q_tmp[j + 1] = it;
                        C += it->processing_time;
                        c[it->job] = C;
                        j++;
                    }

                    auto tmp_out = Q_tmp[0];
                    auto tmp_in = Q_in_tmp[0];

                    if (cmp(tmp_out, tmp_in)) {
                        u_it--;
                    } else {
                        std::swap(Q_tmp[0], Q_tmp[1]);
                        Q_in_tmp[0] = tmp_out;
                        c[tmp_in->job] = c[tmp_out->job] -
                                         tmp_out->processing_time +
                                         tmp_in->processing_time;
                        c[tmp_out->job] =
                            c[tmp_in->job] + tmp_out->processing_time;

                        if (c[tmp_in->job] <= I->b && c[tmp_in->job] > I->a) {
                            u[tmp_out->job] = u_it;
                            u[tmp_in->job] = u_it;
                            auto C_aux = c[tmp_in->job];

                            std::ranges::sort(Q_in_tmp, cmp);
                            for (auto&& [jj, it] :
                                 Q_in_tmp | ranges::views::enumerate) {
                                Q_tmp[jj + 1] = it;
                                C_aux += it->processing_time;
                                c[it->job] = C_aux;
                            }
                            u_it--;
                        } else {
                            if (!(c[tmp_out->job] <= I->b &&
                                  c[tmp_out->job] >
                                      I->a - tmp_out->processing_time)) {
                                Q_in_tmp.erase(
                                    std::remove(Q_in_tmp.begin(),
                                                Q_in_tmp.end(), tmp_out),
                                    Q_in_tmp.end());
                            }
                            u[tmp_out->job] = u_it;
                            Q_tmp.erase(
                                std::remove(Q_tmp.begin(), Q_tmp.end(), tmp_in),
                                Q_tmp.end());
                            auto old_u = u_it - 1;
                            while (old_u + 1 != 0) {
                                auto& tmp_I = intervals[old_u];
                                if (c[tmp_in->job] <= tmp_I->b &&
                                    c[tmp_in->job] > tmp_I->a) {
                                    Q[old_u].push_back(tmp_in);
                                    u[tmp_in->job] = old_u;
                                    if (c[tmp_in->job] <= tmp_I->b &&
                                        c[tmp_in->job] -
                                                tmp_in->processing_time >
                                            tmp_I->a) {
                                        Q_in[old_u].push_back(tmp_in);
                                    }
                                    break;
                                }
                                old_u--;
                            }
                        }
                    }
                } else {
                    auto cmp = compare_edd(I->a, I->b);
                    auto C =
                        c[Q_tmp.front()->job] - Q_tmp.front()->processing_time;
                    std::ranges::sort(Q_tmp, cmp);
                    for (auto& it : Q_tmp) {
                        C += it->processing_time;
                        c[it->job] = C;
                    }
                    u_it--;
                }
            } else {
                u_it--;
            }
        }

        m.job_list.clear();
        m.total_weighted_tardiness = 0;
        m.completion_time = 0;
        for (auto it = 0UL; it < intervals.size(); it++) {
            auto& Q_tmp = Q[it];
            for (auto& job_Q : Q_tmp) {
                m.job_list.push_back(job_Q);
                m.completion_time += job_Q->processing_time;
                c[job_Q->job] = m.completion_time;
                m.total_weighted_tardiness +=
                    job_Q->weighted_tardiness(m.completion_time);
            }
        }

        tw += m.total_weighted_tardiness;
    }
}

void Sol::perturb_swap_inter(size_t l1, size_t l2, std::mt19937& mt) {
    std::uniform_int_distribution dist(0UL, nb_machines - 1);

    auto m1 = dist(mt);
    auto m2 = dist(mt);

    while (m1 == m2) {
        m2 = dist(mt);
    }

    if (l1 >= machines[m1].job_list.size() ||
        l2 >= machines[m2].job_list.size()) {
        return;
    }

    std::uniform_int_distribution dist1(0UL,
                                        machines[m1].job_list.size() - l1 - 1);

    auto i1 = dist1(mt);

    std::uniform_int_distribution dist2(0UL,
                                        machines[m2].job_list.size() - l2 - 1);

    auto i2 = dist2(mt);

    update_swap_move_inter(i1, i2, m1, m2, l1, l2);
}

void Sol::add_job_front_machine(Job* job) {
    ranges::pop_heap(machines, ranges::greater{}, &Machine::completion_time);
    auto& m = machines.back();
    m.add_job(job);
    tw += job->weighted_tardiness(m.completion_time);
    c[job->job] = m.completion_time;
}

void Sol::calculate_partition(const VecIntervalPtr& v) {
    for (auto& m : machines) {
        auto interval_it = v.begin();
        for (auto& job_it : m.job_list) {
            while (!(c[(job_it)->job] <= (*interval_it)->b &&
                     (*interval_it)->a < c[(job_it)->job])) {
            }
        }
    }
}

void Sol::print_solution() const {
    for (auto&& [j, m] : machines | ranges::views::enumerate) {
        fmt::print("Machine {}: ", j);
        for (auto& tmp_j : m.job_list) {
            fmt::print("{} ", tmp_j->job);
        }
        fmt::print("with C = {}, TW = {}, {} jobs\n", m.completion_time,
                   m.total_weighted_tardiness, m.job_list.size());
    }
    fmt::print("with total weighted tardiness {}\n", tw + off);
}

void Machine::add_job(Job* job) {
    job_list.push_back(job);
    completion_time += job->processing_time;
    total_weighted_tardiness += job->weighted_tardiness(completion_time);
}

void Machine::reset_machine(std::vector<int>& c) {
    completion_time = 0;
    total_weighted_tardiness = 0;
    updated = true;

    for (const auto& it : job_list) {
        completion_time += it->processing_time;
        c[it->job] = completion_time;
        total_weighted_tardiness += it->weighted_tardiness(completion_time);
    }
}

Sol::Sol(const Instance& instance)
    : machines{instance.nb_machines},
      nb_jobs{instance.nb_jobs},
      nb_machines{instance.nb_machines},
      c(nb_jobs, -1),
      u(nb_jobs),
      off{instance.off} {}

void Sol::update_insertion_move(size_t i, size_t j, size_t k, size_t l) {
    auto& m = machines[k];
    auto  begin = m.job_list.begin();
    tw -= m.total_weighted_tardiness;

    swap_ranges(begin + i, begin + i + l, begin + j + 1, begin + j + 1);

    m.reset_machine(c);

    tw += m.total_weighted_tardiness;
}

void Sol::update_swap_move(size_t i, size_t j, size_t k, size_t l1, size_t l2) {
    auto& m = machines[k];
    auto& vec_m = machines[k].job_list;
    tw -= m.total_weighted_tardiness;

    auto it1 = vec_m.begin() + i;
    auto it2 = vec_m.begin() + j;

    swap_ranges(it1, it1 + l1, it2, it2 + l2);
    // ranges::swap_ranges(
    //     machines[k].job_list | ranges::views::drop(i) |
    //     ranges::views::take(l1), machines[k].job_list |
    //     ranges::views::drop(j) |
    //         ranges::views::take(l2));

    m.reset_machine(c);

    tw += m.total_weighted_tardiness;
}

void Sol::update_insertion_move_inter(size_t i,
                                      size_t j,
                                      size_t k,
                                      size_t l,
                                      size_t m) {
    auto& m1 = machines[k];
    auto& m2 = machines[l];
    auto  begin1 = m1.job_list.begin();
    auto  begin2 = m2.job_list.begin();

    tw -= m1.total_weighted_tardiness + m2.total_weighted_tardiness;

    m2.job_list.insert(begin2 + j, begin1 + i, begin1 + i + m);
    m1.job_list.erase(begin1 + i, begin1 + i + m);

    m1.reset_machine(c);
    m2.reset_machine(c);

    tw += m1.total_weighted_tardiness + m2.total_weighted_tardiness;
}

void Sol::update_swap_move_inter(size_t i,
                                 size_t j,
                                 size_t k1,
                                 size_t k2,
                                 size_t l1,
                                 size_t l2) {
    auto it1 = machines[k1].job_list.begin() + i;
    auto it2 = machines[k2].job_list.begin() + j;

    // std::ranges::swap_ranges(it1, it1 + l1, it2, it2 + l2);
    ranges::swap_ranges(machines[k1].job_list | ranges::views::drop(i) |
                            ranges::views::take(l1),
                        machines[k2].job_list | ranges::views::drop(j) |
                            ranges::views::take(l2));

    if (l1 < l2) {
        machines[k1].job_list.insert(it1 + l1, it2 + l1, it2 + l2);
        machines[k2].job_list.erase(it2 + l1, it2 + l2);
    } else if (l2 < l1) {
        machines[k2].job_list.insert(it2 + l2, it1 + l2, it1 + l1);
        machines[k1].job_list.erase(it1 + l2, it1 + l1);
    }

    tw -= (machines[k1].total_weighted_tardiness +
           machines[k2].total_weighted_tardiness);
    machines[k1].reset_machine(c);
    machines[k2].reset_machine(c);
    tw += machines[k1].total_weighted_tardiness +
          machines[k2].total_weighted_tardiness;
}
