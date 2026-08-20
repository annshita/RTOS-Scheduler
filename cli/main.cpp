#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "scheduler/JsonExport.hpp"
#include "scheduler/Report.hpp"
#include "scheduler/Simulator.hpp"
#include "scheduler/TaskLoader.hpp"
#include "scheduler/algorithms/EDFScheduler.hpp"
#include "scheduler/algorithms/FPSScheduler.hpp"
#include "scheduler/algorithms/LLFScheduler.hpp"
#include "scheduler/algorithms/RMSScheduler.hpp"
#include "scheduler/algorithms/RoundRobinScheduler.hpp"

using namespace rts;

namespace {

void printUsage() {
    std::cout <<
        "rtsched -- real-time task scheduling simulator\n\n"
        "Usage:\n"
        "  rtsched [--tasks <file.csv>] [--algo <name>] [--quantum <n>]\n"
        "           [--horizon <n>] [--out <file.json>]\n\n"
        "Options:\n"
        "  --tasks <file>   CSV task file (id,name,arrival,exec,period,deadline,priority).\n"
        "                   Defaults to a built-in 3-task example if omitted.\n"
        "  --algo <name>    edf | fps | rms | llf | rr | all   (default: edf)\n"
        "  --quantum <n>    Time slice for Round-Robin (default: 2)\n"
        "  --horizon <n>    Override simulation length in ticks (default: hyperperiod)\n"
        "  --out <file>     Write JSON results for the web viewer (default: web/results.json)\n"
        "  --list           List available algorithms and exit\n"
        "  --help           Show this message\n";
}

std::unique_ptr<Scheduler> makeScheduler(const std::string& algo, int quantum) {
    if (algo == "edf") return std::make_unique<EDFScheduler>();
    if (algo == "fps") return std::make_unique<FPSScheduler>();
    if (algo == "rms") return std::make_unique<RMSScheduler>();
    if (algo == "llf") return std::make_unique<LLFScheduler>();
    if (algo == "rr")  return std::make_unique<RoundRobinScheduler>(quantum);
    return nullptr;
}

} // namespace

int main(int argc, char** argv) {
    std::string tasksPath;
    std::string algo = "edf";
    std::string outPath = "web/results.json";
    int quantum = 2;
    int horizon = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto next = [&](const char* flag) -> std::string {
            if (i + 1 >= argc) throw std::runtime_error(std::string(flag) + " requires a value");
            return argv[++i];
        };
        if (arg == "--tasks") tasksPath = next("--tasks");
        else if (arg == "--algo") algo = next("--algo");
        else if (arg == "--quantum") quantum = std::stoi(next("--quantum"));
        else if (arg == "--horizon") horizon = std::stoi(next("--horizon"));
        else if (arg == "--out") outPath = next("--out");
        else if (arg == "--list") {
            std::cout << "Available algorithms: edf, fps, rms, llf, rr, all\n";
            return 0;
        } else if (arg == "--help") {
            printUsage();
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n\n";
            printUsage();
            return 1;
        }
    }

    std::vector<TaskConfig> tasks;
    try {
        tasks = tasksPath.empty() ? defaultTaskSet() : loadTasksFromCsv(tasksPath);
    } catch (const std::exception& e) {
        std::cerr << "Error loading tasks: " << e.what() << "\n";
        return 1;
    }
    if (tasksPath.empty()) {
        std::cout << "No --tasks file supplied; using the built-in 3-task example.\n";
    }

    Simulator sim(tasks);
    std::vector<SimulationResult> results;

    if (algo == "all") {
        for (const std::string a : {"edf", "fps", "rms", "llf", "rr"}) {
            auto scheduler = makeScheduler(a, quantum);
            results.push_back(sim.run(*scheduler, horizon));
        }
        for (const auto& r : results) printReport(r);
        printComparison(results);
    } else {
        auto scheduler = makeScheduler(algo, quantum);
        if (!scheduler) {
            std::cerr << "Unknown algorithm '" << algo << "'. Use --list to see options.\n";
            return 1;
        }
        results.push_back(sim.run(*scheduler, horizon));
        printReport(results.front());
    }

    try {
        exportResultsToJson(tasks, results, outPath);
        std::cout << "\nResults written to " << outPath
                   << " -- open web/index.html (served over http) to visualize.\n";
    } catch (const std::exception& e) {
        std::cerr << "Warning: could not write JSON output: " << e.what() << "\n";
    }

    return 0;
}
