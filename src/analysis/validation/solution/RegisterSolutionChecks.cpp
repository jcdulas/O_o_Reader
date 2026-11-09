#include "oor/analysis/validation/solution/SolutionChecks.hpp"

namespace oor::analysis
{

void registerSolutionChecks(CheckRegistry& registry)
{
    addMissingSolutionSequenceCheck(registry);
    addUnknownSolutionSequenceCheck(registry);
    addMissingMethodCheck(registry);
    addMissingComplexMethodCheck(registry);
    addMissingFrequencyCheck(registry);
    addMissingTimeStepCheck(registry);
    addMissingSpcCheck(registry);
    addMissingLoadCheck(registry);
    addNoOutputRequestCheck(registry);
    addLanczosEigenMethodSingleRootCheck(registry);
    addTransientTimeStepsOutputIntervalCheck(registry);
    addHeatNoBoundaryCheck(registry);
    addRotorSpinRepeatedCheck(registry);
    addRotorAxisymAxisCheck(registry);
}

} // namespace oor::analysis
