#pragma once
#include <string>
#include <vector>
#include "scheduler/Task.hpp"
#include "scheduler/Types.hpp"

namespace rts {

// Writes the task set plus one or more SimulationResults to a single JSON
// file consumable by web/app.js (a dropdown lets the viewer switch between
// algorithms without re-running the CLI). Throws std::runtime_error if the
// file can't be written.
void exportResultsToJson(const std::vector<TaskConfig>& tasks,
                          const std::vector<SimulationResult>& results,
                          const std::string& outPath);

} // namespace rts
