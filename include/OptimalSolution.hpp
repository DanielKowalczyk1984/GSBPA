#ifndef OPTIMAL_SOLUTION_HPP
#define OPTIMAL_SOLUTION_HPP
#include <job.h>
#include <iostream>

template <typename T = double>
class OptimalSolution {
   public:
    T          obj;
    int        cost;
    int        C_max;
    GPtrArray* jobs;

    /** Default constructor */
    OptimalSolution() : obj(0), cost(0), C_max(0), jobs(g_ptr_array_new()) {}

    explicit OptimalSolution(T _obj)
        : obj(_obj),
          cost(0),
          C_max(0),
          jobs(g_ptr_array_new()) {}

    /** Copy constructor */
    OptimalSolution(const OptimalSolution& other)
        : obj(other.obj),
          cost(other.cost),
          C_max(other.C_max),
          jobs(g_ptr_array_sized_new(other.jobs->len)) {
        for (unsigned i = 0; i < other.jobs->len; ++i) {
            g_ptr_array_add(jobs, g_ptr_array_index(other.jobs, i));
        }

        for (unsigned i = 0; i < other.edges->len; ++i) {
            g_ptr_array_add(jobs, g_ptr_array_index(other.edges, i));
        }
    }

    /** Move constructor */
    OptimalSolution(OptimalSolution&& other) noexcept
        : /* noexcept needed to enable optimizations in containers */
          obj(other.obj),
          cost(other.cost),
          C_max(other.C_max),
          jobs(other.jobs) {
        other.jobs = nullptr;
    }

    /** Copy assignment operator */
    OptimalSolution& operator=(const OptimalSolution& other) {
        OptimalSolution tmp(other);  // re-use copy-constructor
        *this = move(tmp);           // re-use move-assignment
        return *this;
    }

    /** Move assignment operator */
    OptimalSolution& operator=(OptimalSolution&& other) noexcept {
        obj = other.obj;
        cost = other.cost;
        C_max = other.C_max;
        if (jobs) {
            g_ptr_array_free(jobs, TRUE);
        }
        jobs = other.jobs;
        other.jobs = nullptr;
        return *this;
    }

    /** Destructor */
    ~OptimalSolution() noexcept {
        if (jobs) {
            g_ptr_array_free(jobs, TRUE);
        }
    }

    friend std::ostream& operator<<(std::ostream&             os,
                                    OptimalSolution<T> const& o) {
        os << "obj = " << o.obj << "," << std::endl
           << "cost = " << o.cost << " C_max = " << o.C_max << std::endl;
        g_ptr_array_foreach(o.jobs, g_print_machine, NULL);
        return os;
    };

    inline void push_job_back(Job* _job, double _pi) {
        g_ptr_array_add(jobs, _job);
        C_max += _job->processing_time;
        cost += value_Fj(C_max, _job);
        obj += _pi;
    }

    inline void push_job_back_farkas(Job* _job, double _pi) {
        g_ptr_array_add(jobs, _job);
        C_max += _job->processing_time;
        cost += value_Fj(C_max, _job);
        obj += -_pi;
    }

    inline void push_job_back(Job* _job, int C, double _pi) {
        g_ptr_array_add(jobs, _job);
        cost += value_Fj(C + _job->processing_time, _job);
        obj += _pi;
    }

    void reverse_jobs() {
        for (int low = 0, high = jobs->len - 1; low < high; low++, high--) {
            auto temp = g_ptr_array_index(jobs, low);
            g_ptr_array_index(jobs, low) = g_ptr_array_index(jobs, high);
            g_ptr_array_index(jobs, high) = temp;
        }
    }
};

#endif  // OPTIMAL_SOLUTION_HPP
