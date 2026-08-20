#pragma once
#include <limits>
#include "scheduler/Scheduler.hpp"

namespace rts {

// Least Laxity First (LLF), a.k.a. Least Slack Time First: a dynamic
// priority algorithm where laxity = (deadline - now) - remainingTime is
// recomputed every tick, and the job with the smallest laxity (closest to
// missing its deadline given the work still left) runs next. More reactive
// than EDF but prone to frequent context switches ("thrashing") when two
// jobs have near-equal laxity.
class LLFScheduler : public Scheduler {
public:
    std::string name() const override { return "Least Laxity First (LLF)"; }

    int selectTask(const std::vector<TaskInstance>& ready, int currentTime) override {
        int best = -1;
        int bestLaxity = std::numeric_limits<int>::max();
        for (size_t i = 0; i < ready.size(); ++i) {
            const auto& inst = ready[i];
            if (inst.remainingTime <= 0) continue;
            int laxity = (inst.absoluteDeadline - currentTime) - inst.remainingTime;
            if (laxity < bestLaxity ||
                (laxity == bestLaxity && best != -1 && inst.taskId < ready[best].taskId)) {
                best = static_cast<int>(i);
                bestLaxity = laxity;
            }
        }
        return best;
    }
};

} // namespace rts
