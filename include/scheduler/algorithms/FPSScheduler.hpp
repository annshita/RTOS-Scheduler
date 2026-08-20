#pragma once
#include <limits>
#include <unordered_map>
#include "scheduler/Scheduler.hpp"

namespace rts {

// Preemptive Fixed-Priority Scheduling (FPS): each task has a static
// priority (TaskConfig::priority, lower value = higher priority) assigned
// up front, e.g. by a designer or by an offline analysis. At every tick the
// ready job belonging to the highest-priority task runs, preempting any
// lower-priority job currently executing.
class FPSScheduler : public Scheduler {
public:
    std::string name() const override { return "Preemptive Fixed-Priority Scheduling (FPS)"; }

    void initialize(const std::vector<TaskConfig>& tasks) override {
        priority_.clear();
        for (const auto& t : tasks) priority_[t.id] = t.priority;
    }

    int selectTask(const std::vector<TaskInstance>& ready, int) override {
        int best = -1;
        int bestPriority = std::numeric_limits<int>::max();
        for (size_t i = 0; i < ready.size(); ++i) {
            const auto& inst = ready[i];
            if (inst.remainingTime <= 0) continue;
            int p = priorityOf(inst.taskId);
            if (p < bestPriority ||
                (p == bestPriority && best != -1 && inst.taskId < ready[best].taskId)) {
                best = static_cast<int>(i);
                bestPriority = p;
            }
        }
        return best;
    }

protected:
    int priorityOf(int taskId) const {
        auto it = priority_.find(taskId);
        return it == priority_.end() ? std::numeric_limits<int>::max() : it->second;
    }

    std::unordered_map<int, int> priority_;
};

} // namespace rts
