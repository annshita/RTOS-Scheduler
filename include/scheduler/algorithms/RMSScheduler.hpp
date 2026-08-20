#pragma once
#include <algorithm>
#include <vector>
#include "scheduler/algorithms/FPSScheduler.hpp"

namespace rts {

// Rate-Monotonic Scheduling (RMS): a special case of fixed-priority
// scheduling where priority is derived automatically -- the shorter a
// task's period, the higher its priority. Optimal among all static-priority
// algorithms for periodic tasks with deadlines equal to their periods.
class RMSScheduler : public FPSScheduler {
public:
    std::string name() const override { return "Rate-Monotonic Scheduling (RMS)"; }

    void initialize(const std::vector<TaskConfig>& tasks) override {
        std::vector<TaskConfig> sorted = tasks;
        std::sort(sorted.begin(), sorted.end(), [](const TaskConfig& a, const TaskConfig& b) {
            int periodA = a.period > 0 ? a.period : a.relativeDeadline;
            int periodB = b.period > 0 ? b.period : b.relativeDeadline;
            return periodA < periodB;
        });
        priority_.clear();
        for (size_t rank = 0; rank < sorted.size(); ++rank) {
            priority_[sorted[rank].id] = static_cast<int>(rank); // 0 = shortest period = highest priority
        }
    }
};

} // namespace rts
