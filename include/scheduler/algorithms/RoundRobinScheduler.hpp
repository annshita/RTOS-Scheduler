#pragma once
#include <algorithm>
#include <deque>
#include <string>
#include "scheduler/Scheduler.hpp"

namespace rts {

// Round-Robin with time slicing: ready jobs sit in a FIFO queue. The job at
// the head runs for up to `quantum` ticks; if it hasn't finished by then it
// is moved to the back of the queue and the next job runs. Not
// deadline-aware -- included as a classic baseline / fairness-oriented
// contrast to the priority-driven algorithms above.
class RoundRobinScheduler : public Scheduler {
public:
    explicit RoundRobinScheduler(int quantum) : quantum_(quantum < 1 ? 1 : quantum) {}

    std::string name() const override {
        return "Round-Robin (quantum=" + std::to_string(quantum_) + ")";
    }

    void onInstanceReleased(const TaskInstance& inst) override {
        queue_.push_back(inst.instanceId);
    }

    void onInstanceCompleted(const TaskInstance& inst) override {
        auto it = std::find(queue_.begin(), queue_.end(), inst.instanceId);
        if (it != queue_.end()) queue_.erase(it);
        if (inst.instanceId == currentInstanceId_) {
            currentInstanceId_ = -1;
            ticksInSlice_ = 0;
        }
    }

    int selectTask(const std::vector<TaskInstance>& ready, int) override {
        while (!queue_.empty()) {
            int id = queue_.front();
            for (size_t i = 0; i < ready.size(); ++i) {
                if (ready[i].instanceId == id && ready[i].remainingTime > 0) {
                    currentInstanceId_ = id;
                    return static_cast<int>(i);
                }
            }
            queue_.pop_front(); // stale entry, shouldn't normally happen
        }
        currentInstanceId_ = -1;
        return -1;
    }

    void onTick(int, int runningInstanceId) override {
        if (runningInstanceId == -1) return;
        ticksInSlice_++;
        if (ticksInSlice_ >= quantum_) {
            if (!queue_.empty() && queue_.front() == runningInstanceId) {
                queue_.pop_front();
                queue_.push_back(runningInstanceId);
            }
            ticksInSlice_ = 0;
            currentInstanceId_ = -1; // force re-pick (and re-arm slice) next tick
        }
    }

private:
    int quantum_;
    std::deque<int> queue_;
    int currentInstanceId_ = -1;
    int ticksInSlice_ = 0;
};

} // namespace rts
