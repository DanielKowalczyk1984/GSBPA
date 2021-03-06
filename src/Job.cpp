#include "Job.h"
#include <algorithm>

Job::Job(int p, int w, int d) : processing_time(p), due_time(d), weight(w) {}

int Job::weighted_tardiness(int C) {
    return weight * std::max(0, C - due_time);
}

int Job::weighted_tardiness_start(int S) {
    return weight * std::max(0, S + processing_time - due_time);
}

int value_diff_Fij(int C, Job* i, Job* j) {
    auto val = i->weighted_tardiness_start(C - j->processing_time);
    val += j->weighted_tardiness(C + i->processing_time);
    val -= j->weighted_tardiness(C);
    val -= i->weighted_tardiness_start(C);
    return val;
}

int bool_diff_Fij(int weight, Job* _prev, Job* tmp_j) {
    return (_prev == nullptr) ? 1
                              : (value_diff_Fij(weight + tmp_j->processing_time,
                                                _prev, tmp_j) >= 0);
}
