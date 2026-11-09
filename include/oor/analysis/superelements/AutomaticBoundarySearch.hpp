#pragma once

#include "oor/analysis/superelements/BoundaryLink.hpp"
#include "oor/analysis/superelements/PartPlacement.hpp"
#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <map>
#include <vector>

namespace oor::analysis
{

// The automatic search of the boundary of part superelements (SEBULK METHOD = AUTO, SEBNDRY,
// SEEXCLD, CONFAC): every grid point of a part that is within the tolerance of a grid point of the residual structure
// or of another part searched automatically, once both are placed, is joined to it. Points of one part never join each
// other; the grid points of the residual structure take part whether elements use them or not; scalar points have no
// location and need an SECONCT. SEBNDRY restricts the points of SEIDA that are searched against SEIDB (or all others)
// to its list; SEEXCLD keeps its points of SEIDA from joining SEIDB (or any other). The tolerance of each pair of
// superelements is SuperelementCatalogue::searchTolerance.
//
// The pairs are found with a PointCellIndex: time proportional to the number of points.
class AutomaticBoundarySearch
{
public:
    // `residualPoints`: the grid points of the residual structure; `searched`: the placed points of each superelement
    // whose boundary is searched (the parts and images with METHOD = AUTO; for an external superelement the points of
    // its EXTRN). The links go from the point of the superelement with the larger number (the upstream one) to the
    // point it coincides with, by ascending (superelement, point).
    [[nodiscard]] static std::vector<BoundaryLink> search(const SuperelementCatalogue& catalogue,
                                                          const PlacedPoints& residualPoints,
                                                          const std::map<base::EntityId, PlacedPoints>& searched);
};

} // namespace oor::analysis
