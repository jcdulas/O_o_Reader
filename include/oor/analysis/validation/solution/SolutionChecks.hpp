#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// Family 7: solution control. The SOL statement against what the Case Control selects, and the ranges of the solution
// control cards. They judge the deck as a whole, so they run once, on the residual structure.

namespace oor::analysis
{

void addMissingSolutionSequenceCheck(CheckRegistry& registry);
void addUnknownSolutionSequenceCheck(CheckRegistry& registry);
void addMissingMethodCheck(CheckRegistry& registry);
void addMissingComplexMethodCheck(CheckRegistry& registry);
void addMissingFrequencyCheck(CheckRegistry& registry);
void addMissingTimeStepCheck(CheckRegistry& registry);
void addMissingSpcCheck(CheckRegistry& registry);
void addMissingLoadCheck(CheckRegistry& registry);
void addNoOutputRequestCheck(CheckRegistry& registry);
void addLanczosEigenMethodSingleRootCheck(CheckRegistry& registry);
void addTransientTimeStepsOutputIntervalCheck(CheckRegistry& registry);
void addHeatNoBoundaryCheck(CheckRegistry& registry);
void addRotorSpinRepeatedCheck(CheckRegistry& registry);
void addRotorAxisymAxisCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerSolutionChecks(CheckRegistry& registry);

} // namespace oor::analysis
