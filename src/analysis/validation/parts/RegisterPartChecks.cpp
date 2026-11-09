#include "oor/analysis/validation/parts/PartChecks.hpp"

namespace oor::analysis
{

void registerPartChecks(CheckRegistry& registry)
{
    addForeignReferenceCheck(registry);
}

} // namespace oor::analysis
