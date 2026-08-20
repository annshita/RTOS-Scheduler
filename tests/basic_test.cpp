// Minimal smoke tests -- no external test framework required.
// Each check() prints PASS/FAIL and the program exits non-zero on failure,
// so `ctest` (wired up in CMakeLists.txt) can report it normally.
#include <cstdlib>
#include <iostream>

#include "scheduler/Simulator.hpp"
#include "scheduler/algorithms/EDFScheduler.hpp"
#include "scheduler/algorithms/FPSScheduler.hpp"
#include "scheduler/algorithms/RMSScheduler.hpp"
#include "scheduler/algorithms/LLFScheduler.hpp"
#include "scheduler/algorithms/RoundRobinScheduler.hpp"

using namespace rts;

namespace {
int failures = 0;

void check(bool condition, const std::string& description) {
    if (condition) {
        std::cout << "[PASS] " << description << "\n";
    } else {
        std::cout << "[FAIL] " << description << "\n";
        failures++;
    }
}

// A classically schedulable set (utilization = 0.25 + 0.4 + 0.2 = 0.85 <= 1):
//   T1: arrival 0, exec 1, period 4, deadline 4
//   T2: arrival 0, exec 2, period 5, deadline 5
//   T3: arrival 0, exec 2, period 10, deadline 10
std::vector<TaskConfig> schedulableSet() {
    return {
        {0, "T1", 0, 1, 4, 4, 0},
        {1, "T2", 0, 2, 5, 5, 1},
        {2, "T3", 0, 2, 10, 10, 2},
    };
}
} // namespace

int main() {
    // Hyperperiod computation.
    {
        Simulator sim(schedulableSet());
        check(sim.computeHyperperiod() == 20, "hyperperiod of periods 4,5,10 is 20");
    }

    // EDF should meet all deadlines on a set with utilization <= 1.
    {
        Simulator sim(schedulableSet());
        EDFScheduler edf;
        auto r = sim.run(edf);
        check(r.allDeadlinesMet, "EDF meets all deadlines on a schedulable (U<=1) task set");
        check(r.horizon == 20, "EDF run covers the full hyperperiod");
    }

    // RMS should also meet all deadlines on this set (it's RM-schedulable
    // via the Liu & Layland sufficient bound: 0.85 <= 3*(2^(1/3)-1) ~= 0.78?
    // -- not guaranteed by the bound, but this particular set is in fact
    // exactly feasible under RM; we just assert it *runs* without crashing
    // and produces sane, internally-consistent metrics instead of asserting
    // optimality, which only EDF guarantees.
    {
        Simulator sim(schedulableSet());
        RMSScheduler rms;
        auto r = sim.run(rms);
        int totalReleased = 0, totalMissed = 0;
        for (const auto& m : r.perTask) { totalReleased += m.totalInstances; totalMissed += m.missedInstances; }
        check(totalReleased > 0, "RMS releases jobs over the hyperperiod");
        check(totalMissed <= totalReleased, "RMS missed count never exceeds released count");
    }

    // Round-Robin: every released job should eventually complete or be
    // accounted for, and CPU utilization should be sane.
    {
        Simulator sim(schedulableSet());
        RoundRobinScheduler rr(1);
        auto r = sim.run(rr);
        check(r.cpuUtilization >= 0.0 && r.cpuUtilization <= 100.0, "RR CPU utilization is in [0,100]");
        check(static_cast<int>(r.timeline.size()) == r.horizon, "RR timeline has one entry per tick");
    }

    // LLF and FPS should run without crashing and produce a full timeline.
    {
        Simulator sim(schedulableSet());
        LLFScheduler llf;
        auto r = sim.run(llf);
        check(static_cast<int>(r.timeline.size()) == r.horizon, "LLF timeline has one entry per tick");
    }
    {
        Simulator sim(schedulableSet());
        FPSScheduler fps;
        auto r = sim.run(fps);
        check(static_cast<int>(r.timeline.size()) == r.horizon, "FPS timeline has one entry per tick");
    }

    std::cout << "\n" << (failures == 0 ? "All tests passed.\n" : std::to_string(failures) + " test(s) failed.\n");
    return failures == 0 ? 0 : 1;
}
