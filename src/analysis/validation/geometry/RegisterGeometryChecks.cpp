#include "oor/analysis/validation/geometry/GeometryChecks.hpp"

namespace oor::analysis
{

void registerGeometryChecks(CheckRegistry& registry)
{
    addCoincidentNodesCheck(registry);
}

} // namespace oor::analysis
