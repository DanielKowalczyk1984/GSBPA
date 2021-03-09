#ifndef OPTIMAL_SOLUTION_HPP
#define OPTIMAL_SOLUTION_HPP
#include <bits/ranges_algo.h>
#include <iostream>
#include <span>
#include <vector>
#include "Job.h"
#include "fmt/core.h"

template <typename T = double>
class OptimalSolution {
   public:
    T   obj;
    int cost{};
    int C_max{};
    // GPtrArray* jobs{g_ptr_array_new()};
    std::vector<Job*> jobs{};

    /** Default constructor */
    OptimalSolution() : obj(0) {}

    explicit OptimalSolution(T _obj) : obj(_obj) {}

    /** Copy constructor */
    OptimalSolution(const OptimalSolution& other) = default;
    // : obj(other.obj),
    //   cost(other.cost),
    //   C_max(other.C_max),
    //   jobs(g_ptr_array_sized_new(other.jobs->len)) {
    //     for (unsigned i = 0; i < other.jobs->len; ++i) {
    //         g_ptr_array_add(jobs, g_ptr_array_index(other.jobs, i));
    //     }

    //     for (unsigned i = 0; i < other.edges->len; ++i) {
    //         g_ptr_array_add(jobs, g_ptr_array_index(other.edges, i));
    //     }
    // }

    /** Move constructor */
    OptimalSolution(OptimalSolution&& other) = default;

    // noexcept
    //     : /* noexcept needed to enable optimizations in containers */
    //       obj(other.obj),
    //       cost(other.cost),
    //       C_max(other.C_max),
    //       jobs(other.jobs) {
    //     other.jobs = nullptr;
    // }

    /** Copy assignment operator */
    OptimalSolution& operator=(const OptimalSolution& other) = default;

    //     {
    //     OptimalSolution tmp(other);  // re-use copy-constructor
    //     *this = move(tmp);           // re-use move-assignment
    //     return *this;
    // }

    /** Move assignment operator */
    OptimalSolution& operator=(OptimalSolution&& other) = default;

    //     noexcept {
    //     obj = other.obj;
    //     cost = other.cost;
    //     C_max = other.C_max;
    //     if (jobs) {
    //         g_ptr_array_free(jobs, TRUE);
    //     }
    //     jobs = other.jobs;
    //     other.jobs = nullptr;
    //     return *this;
    // }

    /** Destructor */
    ~OptimalSolution() = default;
    // noexcept {
    // if (jobs) {
    //     g_ptr_array_free(jobs, TRUE);
    // }

    friend std::ostream& operator<<(std::ostream&             os,
                                    OptimalSolution<T> const& o) {
        os << "obj = " << o.obj << "," << std::endl
           << "cost = " << o.cost << " C_max = " << o.C_max << std::endl;
        // g_ptr_array_foreach(o.jobs, g_print_machine, NULL);

        for (auto& it : o.jobs) {
            os << it->job << ' ';
        }
        //   [&os](const Job& tmp) { os << tmp.job << " "; });
        return os;
    };

    inline void push_job_back(Job* _job, double _pi) {
        // g_ptr_array_add(jobs, _job);
        jobs.push_back(_job);
        C_max += _job->processing_time;
        cost += value_Fj(C_max, _job);
        obj += _pi;
    }

    inline void push_job_back_farkas(Job* _job, double _pi) {
        // g_ptr_array_add(jobs, _job);
        jobs.push_back(_job);
        C_max += _job->processing_time;
        cost += value_Fj(C_max, _job);
        obj += -_pi;
    }

    inline void push_job_back(Job* _job, int C, double _pi) {
        // g_ptr_array_add(jobs, _job);
        jobs.push_back(_job);
        cost += value_Fj(C + _job->processing_time, _job);
        obj += _pi;
    }

    void reverse_jobs() {
        std::ranges::reverse(jobs);
        // std::span aux{jobs->pdata, jobs->len};
        // for (int low = 0, high = jobs->len - 1; low < high; low++, high--) {
        //     auto temp = aux[low];
        //     aux[low] = aux[high];
        //     aux[high] = temp;
        // }
    }
};

#endif  // OPTIMAL_SOLUTION_HPP
