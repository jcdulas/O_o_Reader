#include "oor/analysis/validation/coordinates/CoordinateChecks.hpp"

namespace oor::analysis
{

void registerCoordinateChecks(CheckRegistry& registry)
{
    addCoordinateCycleCheck(registry);
    addCoordinateDegenerateCheck(registry);
    addNodeOnAxisCheck(registry);
}

} // namespace oor::analysis
