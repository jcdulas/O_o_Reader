#include "oor/analysis/superelements/BoundaryLink.hpp"

namespace oor::analysis
{

std::string_view boundarySourceName(BoundarySource source) noexcept
{
    switch (source)
    {
    case BoundarySource::Automatic:
        return "automatic";
    case BoundarySource::SuperelementConnection:
        return "SuperelementConnection";
    case BoundarySource::SuperelementBoundarySearch:
        return "SuperelementBoundarySearch";
    case BoundarySource::SuperelementExteriorPoints:
        return "SuperelementExteriorPoints";
    case BoundarySource::SuperelementInteriorNodes:
        return "SuperelementInteriorNodes";
    case BoundarySource::SuperelementExternalBoundary:
        return "SuperelementExternalBoundary";
    case BoundarySource::SuperelementCopyPoints:
        return "SuperelementCopyPoints";
    }
    return "automatic";
}

} // namespace oor::analysis
