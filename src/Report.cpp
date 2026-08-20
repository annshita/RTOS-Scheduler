#include "scheduler/Report.hpp"

#include <iomanip>
#include <iostream>

namespace rts {

void printReport(const SimulationResult& r) {
    std::cout << "\n=== " << r.algorithmName << " ===\n";
    std::cout << "Horizon: " << r.horizon << " ticks | Idle: " << r.idleTime
               << " | Context switches: " << r.contextSwitches << "\n";
    std::cout << "CPU Utilization: " << std::fixed << std::setprecision(1)
               << r.cpuUtilization << "%\n";
    std::cout << (r.allDeadlinesMet ? "Result: \xE2\x9C\x85 all deadlines met\n"
                                     : "Result: \xE2\x9A\xA0  one or more deadlines missed\n");

    std::cout << "\n" << std::left
               << std::setw(4) << "ID" << std::setw(12) << "Name"
               << std::setw(10) << "Released" << std::setw(9) << "Missed"
               << std::setw(11) << "MissRate" << std::setw(12) << "AvgResp"
               << "CPU Time\n";
    std::cout << std::string(70, '-') << "\n";
    for (const auto& m : r.perTask) {
        std::cout << std::left
                   << std::setw(4) << m.taskId
                   << std::setw(12) << m.name
                   << std::setw(10) << m.totalInstances
                   << std::setw(9) << m.missedInstances
                   << std::setw(10) << std::fixed << std::setprecision(1) << m.missRate << "%"
                   << std::setw(1) << " "
                   << std::setw(11) << std::fixed << std::setprecision(2) << m.avgResponseTime
                   << m.totalRunTime << "\n";
    }
}

void printComparison(const std::vector<SimulationResult>& results) {
    std::cout << "\n=== Algorithm Comparison ===\n";
    std::cout << std::left
               << std::setw(42) << "Algorithm" << std::setw(10) << "CPU %"
               << std::setw(14) << "Missed" << std::setw(16) << "Ctx Switches"
               << "All Met\n";
    std::cout << std::string(90, '-') << "\n";
    for (const auto& r : results) {
        int totalMissed = 0;
        for (const auto& m : r.perTask) totalMissed += m.missedInstances;
        std::cout << std::left
                   << std::setw(42) << r.algorithmName
                   << std::setw(10) << std::fixed << std::setprecision(1) << r.cpuUtilization
                   << std::setw(14) << totalMissed
                   << std::setw(16) << r.contextSwitches
                   << (r.allDeadlinesMet ? "yes" : "no") << "\n";
    }
}

} // namespace rts
