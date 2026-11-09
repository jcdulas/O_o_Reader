#pragma once

#include "oor/analysis/superelements/BoundaryLink.hpp"
#include "oor/analysis/superelements/PartPlacement.hpp"
#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace oor::analysis
{

// The connections the cards give point by point:
//   - SuperelementConnection (format reference SECONCT): GIDAi of SEIDA joined to GIDBi of SEIDB (0: the residual
//     structure), a THRU range to a THRU range one by one; with LOC = YES a pair farther apart than the tolerance after
//     placement is a fatal error of the solver (written to `problems`, the pair kept), with LOC = NO the points are
//     joined wherever they are;
//   - SuperelementExternalBoundary (format reference EXTRN): the boundary points of an external superelement and their
//     components, in its part;
//   - SuperelementCopyPoints (format reference CSUPER, SEQSEP): the exterior points GPi of a secondary superelement, in
//     the superelement downstream of it, paired with the exterior points of its primary in the primary's order
//     (ascending ID), or in the order of its SuperelementCopySequence.
class ManualConnections
{
public:
    // `placed`: the placed points of the superelements that have part points (for the distance of the pairs), the
    // residual structure under 0.
    ManualConnections(const SuperelementCatalogue& catalogue, const std::map<base::EntityId, PlacedPoints>& placed);

    // The SuperelementConnection pairs, by card then by pair.
    [[nodiscard]] const std::vector<BoundaryLink>& links() const noexcept { return m_links; }
    // The SuperelementExternalBoundary points of the external superelements with their components as text ("123456",
    // "0" for a scalar point).
    [[nodiscard]] const std::map<SuperelementPoint, std::string>& externalPoints() const noexcept
    {
        return m_externalPoints;
    }
    [[nodiscard]] const std::vector<std::string>& problems() const noexcept { return m_problems; }

    // The SuperelementCopyPoints pairs. `exteriorOf(seid)` gives the exterior points of a primary superelement by
    // ascending ID (they are known once the other connections are joined).
    [[nodiscard]] static std::vector<BoundaryLink> superelementCopyPointsLinks(
        const SuperelementCatalogue& catalogue,
        const std::function<std::vector<base::EntityId>(base::EntityId)>& exteriorOf,
        std::vector<std::string>& problems);

private:
    std::vector<BoundaryLink> m_links;
    std::map<SuperelementPoint, std::string> m_externalPoints;
    std::vector<std::string> m_problems;
};

} // namespace oor::analysis
