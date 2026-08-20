#pragma once
#include <string>
#include <vector>
#include "scheduler/Task.hpp"

namespace rts {

// One concrete release ("job") of a task during the simulation.
struct TaskInstance {
    int instanceId = -1;
    int taskId = -1;
    int arrivalTime = 0;
    int absoluteDeadline = 0;
    int execTime = 0;
    int remainingTime = 0;
    bool completed = false;
    bool missed = false;
    int startTime = -1;   // first tick it was ever run on the CPU
    int finishTime = -1;  // tick at which it completed
};

// One tick of the produced schedule (for Gantt-chart style output).
// taskId == -1 means the CPU was idle at this tick.
struct ScheduleEvent {
    int time = 0;
    int taskId = -1;
    int instanceId = -1;
};

struct TaskMetrics {
    int taskId = 0;
    std::string name;
    int totalInstances = 0;
    int missedInstances = 0;
    double missRate = 0.0;          // percent
    double avgResponseTime = 0.0;   // ticks, start - arrival
    int totalRunTime = 0;           // ticks actually executed
};

struct SimulationResult {
    std::string algorithmName;
    int horizon = 0;
    int idleTime = 0;
    int contextSwitches = 0;
    double cpuUtilization = 0.0;    // percent
    bool allDeadlinesMet = true;
    std::vector<ScheduleEvent> timeline;
    std::vector<TaskMetrics> perTask;
};

} // namespace rts
