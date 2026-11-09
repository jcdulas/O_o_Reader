#pragma once

#include "oor/analysis/superelements/BoundaryLink.hpp"
#include "oor/analysis/superelements/ListSuperelementPartition.hpp"
#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::analysis
{

// One boundary point of one superelement (a row of `part_boundaries`).
struct PartBoundaryRow
{
    base::EntityId seid = 0;
    base::EntityId nid = 0; // in the superelement's own IDs (an image: its primary's)
    base::EntityId downstreamId = 0;
    // The point of the downstream superelement it is; nullopt when the downstream superelement has none there and the solver
    // creates one (labelled 2 000 000 000 + boundaryPoint in the solver's GPLS table).
    std::optional<base::EntityId> downstreamNid;
    std::int32_t boundaryPoint = 0; // the solver's boundary sequence number, the key that ties the points of one together
    BoundarySource source = BoundarySource::Automatic;
    std::string dof;      // the components it connects: "123456", "0" for a scalar point
    double distance = 0.; // to the downstream point (else to the nearest point it joins) after placement; NaN for none
};

// The boundary of every superelement of a deck: the part superelements by
// their SECONCT cards or the automatic search (PartPlacement, AutomaticBoundarySearch, ManualConnections), the
// external ones by their EXTRN, the list superelements by their elements (ListSuperelementPartition), the secondary
// ones by their CSUPER; the points joined into boundary points numbered as the solver numbers them (BoundaryPointNumbering).
//
// Computed once, when made, from the residual structure of the deck the model belongs to and its parts, in time
// proportional to the number of points and elements; nothing is kept in the model (a query makes one per call). The
// object holds pointers into the deck and is valid while the deck is not edited.
class PartBoundaries
{
public:
    explicit PartBoundaries(const model::Model& model);

    [[nodiscard]] const SuperelementCatalogue& catalogue() const noexcept { return m_catalogue; }
    [[nodiscard]] const ListSuperelementPartition& listPartition() const noexcept { return m_listPartition; }
    // By superelement, then by point.
    [[nodiscard]] const std::vector<PartBoundaryRow>& rows() const noexcept { return m_rows; }
    // What could not be applied: a SELOC or SEMPLN, an SECONCT pair farther apart than its tolerance or that names no
    // point, an element on interior points of two list superelements, a CSUPER of another length than its primary's.
    [[nodiscard]] const std::vector<std::string>& problems() const noexcept { return m_problems; }

private:
    SuperelementCatalogue m_catalogue;
    ListSuperelementPartition m_listPartition;
    std::vector<PartBoundaryRow> m_rows;
    std::vector<std::string> m_problems;
};

} // namespace oor::analysis
