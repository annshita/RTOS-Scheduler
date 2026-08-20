#include "scheduler/TaskLoader.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace rts {

namespace {
std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ',')) {
        // trim whitespace
        size_t start = field.find_first_not_of(" \t\r\n");
        size_t end = field.find_last_not_of(" \t\r\n");
        fields.push_back(start == std::string::npos ? "" : field.substr(start, end - start + 1));
    }
    return fields;
}

bool isInteger(const std::string& s) {
    if (s.empty()) return false;
    size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    for (; i < s.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
    return true;
}
} // namespace

std::vector<TaskConfig> loadTasksFromCsv(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open task file: " + path);
    }

    std::vector<TaskConfig> tasks;
    std::string line;
    int rowIndex = 0;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t firstNonSpace = line.find_first_not_of(" \t\r\n");
        if (firstNonSpace == std::string::npos || line[firstNonSpace] == '#') continue;

        auto fields = splitCsvLine(line);
        if (fields.size() < 6) continue; // skip malformed / header rows
        if (!isInteger(fields[0])) continue; // header row (id column isn't numeric)

        TaskConfig cfg;
        cfg.id = std::stoi(fields[0]);
        cfg.name = fields[1].empty() ? ("Task" + std::to_string(cfg.id)) : fields[1];
        cfg.arrival = std::stoi(fields[2]);
        cfg.execTime = std::stoi(fields[3]);
        cfg.period = std::stoi(fields[4]);
        cfg.relativeDeadline = std::stoi(fields[5]);
        cfg.priority = (fields.size() >= 7 && isInteger(fields[6])) ? std::stoi(fields[6]) : rowIndex;

        tasks.push_back(cfg);
        rowIndex++;
    }

    if (tasks.empty()) {
        throw std::runtime_error("No valid task rows found in: " + path);
    }

    return tasks;
}

std::vector<TaskConfig> defaultTaskSet() {
    return {
        {0, "Sensor",  0, 2, 5, 4, 0},
        {1, "Control", 1, 3, 6, 5, 1},
        {2, "Comm",    2, 1, 4, 3, 2},
    };
}

} // namespace rts
