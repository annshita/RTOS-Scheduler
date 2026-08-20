#pragma once
#include <string>
#include <vector>
#include "scheduler/Task.hpp"

namespace rts {

// Loads a task set from a simple CSV file:
//   id,name,arrival,exec,period,deadline,priority
// - Lines starting with '#' and blank lines are ignored.
// - A header row is auto-detected and skipped.
// - `priority` is optional; if omitted it defaults to the row index.
// Throws std::runtime_error on malformed input or missing file.
std::vector<TaskConfig> loadTasksFromCsv(const std::string& path);

// A small built-in task set (mirrors the original 3-task example) used
// when no task file is supplied.
std::vector<TaskConfig> defaultTaskSet();

} // namespace rts
