#include "scheduler/JsonExport.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace rts {

namespace {
std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            default: out += c;
        }
    }
    return out;
}

void writeTaskConfig(std::ostringstream& os, const TaskConfig& t) {
    os << "{\"id\":" << t.id
       << ",\"name\":\"" << jsonEscape(t.name) << "\""
       << ",\"arrival\":" << t.arrival
       << ",\"execTime\":" << t.execTime
       << ",\"period\":" << t.period
       << ",\"relativeDeadline\":" << t.relativeDeadline
       << ",\"priority\":" << t.priority
       << "}";
}

void writeResult(std::ostringstream& os, const SimulationResult& r) {
    os << "{";
    os << "\"algorithm\":\"" << jsonEscape(r.algorithmName) << "\",";
    os << "\"horizon\":" << r.horizon << ",";
    os << "\"idleTime\":" << r.idleTime << ",";
    os << "\"contextSwitches\":" << r.contextSwitches << ",";
    os << "\"cpuUtilization\":" << r.cpuUtilization << ",";
    os << "\"allDeadlinesMet\":" << (r.allDeadlinesMet ? "true" : "false") << ",";

    os << "\"timeline\":[";
    for (size_t i = 0; i < r.timeline.size(); ++i) {
        const auto& e = r.timeline[i];
        os << "{\"t\":" << e.time << ",\"taskId\":" << e.taskId << ",\"instanceId\":" << e.instanceId << "}";
        if (i + 1 < r.timeline.size()) os << ",";
    }
    os << "],";

    os << "\"metrics\":[";
    for (size_t i = 0; i < r.perTask.size(); ++i) {
        const auto& m = r.perTask[i];
        os << "{\"taskId\":" << m.taskId
           << ",\"name\":\"" << jsonEscape(m.name) << "\""
           << ",\"totalInstances\":" << m.totalInstances
           << ",\"missedInstances\":" << m.missedInstances
           << ",\"missRate\":" << m.missRate
           << ",\"avgResponseTime\":" << m.avgResponseTime
           << ",\"totalRunTime\":" << m.totalRunTime
           << "}";
        if (i + 1 < r.perTask.size()) os << ",";
    }
    os << "]";
    os << "}";
}
} // namespace

void exportResultsToJson(const std::vector<TaskConfig>& tasks,
                          const std::vector<SimulationResult>& results,
                          const std::string& outPath) {
    std::ostringstream os;
    os << "{\"tasks\":[";
    for (size_t i = 0; i < tasks.size(); ++i) {
        writeTaskConfig(os, tasks[i]);
        if (i + 1 < tasks.size()) os << ",";
    }
    os << "],\"results\":[";
    for (size_t i = 0; i < results.size(); ++i) {
        writeResult(os, results[i]);
        if (i + 1 < results.size()) os << ",";
    }
    os << "]}";

    std::ofstream file(outPath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not write output file: " + outPath);
    }
    file << os.str();
}

} // namespace rts
