#pragma once
#include <limits>
#include "scheduler/Scheduler.hpp"

namespace rts {

// Earliest Deadline First (dynamic priority): the ready job with the
// nearest absolute deadline always runs. Optimal for uniprocessor systems
// in the sense that if any schedule meets all deadlines, EDF does too.
class EDFScheduler : public Scheduler {
public:
    std::string name() const override { return "Earliest Deadline First (EDF)"; }

    int selectTask(const std::vector<TaskInstance>& ready, int) override {
        int best = -1;
        int bestDeadline = std::numeric_limits<int>::max();
        for (size_t i = 0; i < ready.size(); ++i) {
            const auto& inst = ready[i];
            if (inst.remainingTime <= 0) continue;
            if (inst.absoluteDeadline < bestDeadline ||
                (inst.absoluteDeadline == bestDeadline && best != -1 &&
                 inst.taskId < ready[best].taskId)) {
                best = static_cast<int>(i);
                bestDeadline = inst.absoluteDeadline;
            }
        }
        return best;
    }
};

} // namespace rts
