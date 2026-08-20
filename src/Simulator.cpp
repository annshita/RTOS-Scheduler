#include "scheduler/Simulator.hpp"

#include <algorithm>
#include <limits>
#include <numeric>
#include <unordered_map>

namespace rts {

namespace {
int gcdInt(int a, int b) {
    while (b != 0) { int t = b; b = a % b; a = t; }
    return a;
}
int lcmInt(int a, int b) {
    if (a == 0 || b == 0) return std::max(a, b);
    return a / gcdInt(a, b) * b;
}
} // namespace

Simulator::Simulator(std::vector<TaskConfig> tasks) : tasks_(std::move(tasks)) {}

int Simulator::computeHyperperiod() const {
    int result = 1;
    bool any = false;
    for (const auto& t : tasks_) {
        if (t.period > 0) {
            result = lcmInt(result, t.period);
            any = true;
        }
    }
    if (!any) {
        // All tasks are one-shot: horizon just needs to cover the latest deadline.
        int maxDeadline = 0;
        for (const auto& t : tasks_) maxDeadline = std::max(maxDeadline, t.arrival + t.relativeDeadline);
        return std::max(maxDeadline, 1);
    }
    return result;
}

SimulationResult Simulator::run(Scheduler& scheduler, int horizonOverride) const {
    SimulationResult result;
    result.algorithmName = scheduler.name();
    const int horizon = horizonOverride > 0 ? horizonOverride : computeHyperperiod();
    result.horizon = horizon;

    scheduler.initialize(tasks_);

    std::unordered_map<int, int> nextRelease; // taskId -> next release time (-1 = no more releases)
    for (const auto& t : tasks_) nextRelease[t.id] = t.arrival;

    std::vector<TaskInstance> ready;
    int nextInstanceId = 0;

    struct Accum {
        std::string name;
        int totalInstances = 0;
        int missedInstances = 0;
        long long responseTimeSum = 0;
        int totalRunTime = 0;
    };
    std::unordered_map<int, Accum> perTask;
    for (const auto& t : tasks_) perTask[t.id] = Accum{t.name, 0, 0, 0, 0};

    int idleTime = 0;
    int contextSwitches = 0;
    int previousRunningTaskId = std::numeric_limits<int>::min();

    for (int t = 0; t < horizon; ++t) {
        // 1) Release any jobs due at this tick.
        for (const auto& cfg : tasks_) {
            auto it = nextRelease.find(cfg.id);
            if (it == nextRelease.end() || it->second < 0 || it->second != t) continue;

            TaskInstance inst;
            inst.instanceId = nextInstanceId++;
            inst.taskId = cfg.id;
            inst.arrivalTime = t;
            inst.absoluteDeadline = t + cfg.relativeDeadline;
            inst.execTime = cfg.execTime;
            inst.remainingTime = cfg.execTime;
            ready.push_back(inst);
            perTask[cfg.id].totalInstances++;
            scheduler.onInstanceReleased(inst);

            if (cfg.period > 0) it->second += cfg.period;
            else it->second = -1; // one-shot, no further releases
        }

        // 2) Ask the scheduler who runs.
        int selectedIdx = scheduler.selectTask(ready, t);
        int runningInstanceId = -1;
        int runningTaskId = -1;

        if (selectedIdx >= 0 && selectedIdx < static_cast<int>(ready.size())) {
            TaskInstance& inst = ready[static_cast<size_t>(selectedIdx)];
            if (inst.startTime == -1) {
                inst.startTime = t;
                perTask[inst.taskId].responseTimeSum += (inst.startTime - inst.arrivalTime);
            }
            inst.remainingTime--;
            perTask[inst.taskId].totalRunTime++;
            runningInstanceId = inst.instanceId;
            runningTaskId = inst.taskId;

            result.timeline.push_back({t, inst.taskId, inst.instanceId});

            if (inst.remainingTime == 0) {
                inst.completed = true;
                inst.finishTime = t + 1;
                if (inst.finishTime > inst.absoluteDeadline && !inst.missed) {
                    inst.missed = true;
                    perTask[inst.taskId].missedInstances++;
                    result.allDeadlinesMet = false;
                }
                scheduler.onInstanceCompleted(inst);
                ready.erase(ready.begin() + selectedIdx);
            }
        } else {
            idleTime++;
            result.timeline.push_back({t, -1, -1});
        }

        // 3) Catch deadline misses for jobs that are still waiting/running
        //    but have just passed their deadline without finishing.
        for (auto& inst : ready) {
            if (!inst.missed && (t + 1) > inst.absoluteDeadline) {
                inst.missed = true;
                perTask[inst.taskId].missedInstances++;
                result.allDeadlinesMet = false;
            }
        }

        if (runningTaskId != previousRunningTaskId) {
            contextSwitches++;
            previousRunningTaskId = runningTaskId;
        }

        scheduler.onTick(t, runningInstanceId);
    }

    result.idleTime = idleTime;
    result.contextSwitches = std::max(0, contextSwitches - 1); // don't count the initial "switch into" tick 0
    result.cpuUtilization = horizon > 0 ? 100.0 * (horizon - idleTime) / horizon : 0.0;

    for (const auto& cfg : tasks_) {
        const Accum& a = perTask[cfg.id];
        TaskMetrics m;
        m.taskId = cfg.id;
        m.name = a.name;
        m.totalInstances = a.totalInstances;
        m.missedInstances = a.missedInstances;
        m.missRate = a.totalInstances > 0 ? 100.0 * a.missedInstances / a.totalInstances : 0.0;
        m.avgResponseTime = a.totalInstances > 0 ? static_cast<double>(a.responseTimeSum) / a.totalInstances : 0.0;
        m.totalRunTime = a.totalRunTime;
        result.perTask.push_back(m);
    }

    return result;
}

} // namespace rts
