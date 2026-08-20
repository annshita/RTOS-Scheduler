#pragma once
#include <vector>
#include "scheduler/Types.hpp"

namespace rts {

// Prints a human-readable report of a single simulation run to stdout.
void printReport(const SimulationResult& result);

// Prints a compact side-by-side comparison table across several algorithms
// run on the same task set (used by `--algo all`).
void printComparison(const std::vector<SimulationResult>& results);

} // namespace rts
