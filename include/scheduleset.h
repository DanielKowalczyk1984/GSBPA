#ifndef SCHEDULESET_H
#define SCHEDULESET_H
// #ifdef __cplusplus
// extern "C" {
// #endif
// #include <glib.h>
// #include <interval.h>
#include <memory>
#include <vector>
#include "Solution.hpp"

struct ScheduleSet {
    int age{};
    int del{};
    int total_processing_time{};
    int total_weighted_completion_time{};

    std::vector<Job*> job_list{};

    ScheduleSet() = default;
    explicit ScheduleSet(const Machine&);
    ~ScheduleSet() = default;
    ScheduleSet(ScheduleSet&&) = default;
    ScheduleSet& operator=(ScheduleSet&&) = default;
    ScheduleSet(const ScheduleSet&) = default;
    ScheduleSet& operator=(const ScheduleSet&) = default;

    bool operator<(ScheduleSet const& other);
    bool operator==(ScheduleSet const& other) const;

    void recalculate();
};

namespace std {
template <>
struct less<std::shared_ptr<ScheduleSet>> {
    constexpr bool operator()(auto const& lhs, auto const& rhs) {
        return (*lhs) < (*rhs);  // or use boost::hash_combine
    }
};

}  // namespace std

#endif  // SCHEDULESET_H
