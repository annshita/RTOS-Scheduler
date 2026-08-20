#pragma once
#include <vector>
#include "scheduler/Scheduler.hpp"
#include "scheduler/Task.hpp"
#include "scheduler/Types.hpp"

namespace rts {

// Discrete-time, single-CPU scheduling simulator. Given a static task set
// and any Scheduler strategy, runs a tick-by-tick simulation over one
// hyperperiod (LCM of all periods) and produces a full timeline + metrics.
class Simulator {
public:
    explicit Simulator(std::vector<TaskConfig> tasks);

    // horizonOverride: simulate exactly this many ticks instead of the
    // computed hyperperiod (0 => auto).
    SimulationResult run(Scheduler& scheduler, int horizonOverride = 0) const;

    int computeHyperperiod() const;

private:
    std::vector<TaskConfig> tasks_;
};

} // namespace rts
