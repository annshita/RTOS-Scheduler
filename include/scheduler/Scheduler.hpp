#pragma once
#include <string>
#include <vector>
#include "scheduler/Task.hpp"
#include "scheduler/Types.hpp"

namespace rts {

// Strategy interface every scheduling algorithm implements. The Simulator
// drives one tick at a time and only ever hands the scheduler the set of
// currently-ready (released, not yet completed) task instances -- the
// scheduler decides which one (if any) gets the CPU for that tick.
class Scheduler {
public:
    virtual ~Scheduler() = default;

    virtual std::string name() const = 0;

    // Called once before the simulation starts with the static task set.
    // Useful for algorithms that need to precompute static priorities
    // (e.g. Rate-Monotonic).
    virtual void initialize(const std::vector<TaskConfig>& /*tasks*/) {}

    // Called the instant a new job/instance is released.
    virtual void onInstanceReleased(const TaskInstance& /*inst*/) {}

    // Called the instant a job/instance finishes (completes or is retired).
    virtual void onInstanceCompleted(const TaskInstance& /*inst*/) {}

    // Return the index into `ready` to run this tick, or -1 to idle.
    virtual int selectTask(const std::vector<TaskInstance>& ready, int currentTime) = 0;

    // Called after the tick's execution decision has been made, with the
    // instanceId that ran (-1 if idle). Lets stateful schedulers (e.g.
    // Round-Robin) track quantum usage.
    virtual void onTick(int /*currentTime*/, int /*runningInstanceId*/) {}
};

} // namespace rts
