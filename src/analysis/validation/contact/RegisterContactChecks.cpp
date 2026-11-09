#include "oor/analysis/validation/contact/ContactChecks.hpp"

namespace oor::analysis
{

void registerContactChecks(CheckRegistry& registry)
{
    addBodyIdRepeatedCheck(registry);
    addSurfaceIdRepeatedCheck(registry);
    addTableIdRepeatedCheck(registry);
    addBodySurfaceKindCheck(registry);
    addSlidelineSolutionCheck(registry);
    addBrakeBodyKindCheck(registry);
    addBrakeSquealIdRepeatedCheck(registry);
    addPairApartCheck(registry);
}

} // namespace oor::analysis
