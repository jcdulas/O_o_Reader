#include "oor/analysis/validation/references/ReferenceChecks.hpp"

namespace oor::analysis
{

void registerReferenceChecks(CheckRegistry& registry)
{
    addUnresolvedReferenceCheck(registry);
    addDuplicateIdCheck(registry);
    addOrphanPropertyCheck(registry);
    addOrphanMaterialCheck(registry);
    addOrphanCoordinateCheck(registry);
    addOrphanNodeCheck(registry);
}

} // namespace oor::analysis
