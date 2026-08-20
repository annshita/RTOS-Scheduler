#pragma once
#include <string>

namespace rts {

// Static description of a (periodic or one-shot) real-time task, as supplied
// by the user via a task file or built programmatically.
struct TaskConfig {
    int id = 0;
    std::string name;
    int arrival = 0;            // first release time
    int execTime = 0;           // worst-case execution time (WCET), in ticks
    int period = 0;             // 0 (or <=0) => one-shot / aperiodic task
    int relativeDeadline = 0;   // deadline relative to each instance's arrival
    int priority = 0;           // static priority, lower value = higher priority
                                 // (only consulted by Fixed-Priority Scheduling)
};

} // namespace rts
