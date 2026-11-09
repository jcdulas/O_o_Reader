#pragma once

#include "oor/analysis/superelements/BoundaryLink.hpp"
#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <map>
#include <string>
#include <vector>

namespace oor::analysis
{

// An interior grid point of a list superelement, and whether a SuperelementInteriorNodes lists it (else the SEID of
// its Node).
struct InteriorPoint
{
    base::EntityId seid = 0;
    base::EntityId nid = 0;
    bool fromInteriorNodes = false;
};

// An element of a list superelement (its card type is a type index of the registry), and whether a
// SuperelementElemTransfer gives it (else it touches the interior points of the superelement).
struct PartitionElement
{
    base::EntityId seid = 0;
    base::EntityId eid = 0;
    std::uint16_t type = 0;
    bool fromElemTransfer = false;
};

// The partition of the main Bulk Data into list superelements (format reference SESET, GRID, SEELT, CSUPEXT, RELEASE):
//   - interior points of superelement n: the grid points a SuperelementInteriorNodes n lists, else those whose Node
//     (or NodeDefaults) SEID is n; a point interior to no superelement is a residual point;
//   - elements: an element (structural, rigid, scalar, ...: the elements of ElementTopology; the MPC equations do not
//     connect) touching an interior point of n belongs to n; one touching only points interior to no superelement
//     belongs to the residual structure, unless a SuperelementElemTransfer gives it to a superelement;
//   - exterior points of n: the points of its elements that are not interior to it, and the points of the
//     SuperelementExteriorPoints n;
//   - degrees of freedom: all of a grid point but the components a SuperelementReleasedDofs n frees; the one of a
//     scalar point.
// A list superelement shares the ID space of the main Bulk Data: its exterior point is the point of the same ID in the
// superelement downstream of it.
class ListSuperelementPartition
{
public:
    explicit ListSuperelementPartition(const SuperelementCatalogue& catalogue);

    // One link per exterior point: (n, nid) to (downstream of n, nid), with no location; source
    // SuperelementExteriorPoints for a point such a card names, SuperelementInteriorNodes otherwise.
    [[nodiscard]] const std::vector<BoundaryLink>& links() const noexcept { return m_links; }
    // The degrees of freedom of an exterior point as text ("123456", "0" for a scalar point); empty when it is not one.
    [[nodiscard]] std::string dofOf(const SuperelementPoint& point) const;
    // The interior points and the elements of a list superelement.
    [[nodiscard]] std::size_t interiorCount(base::EntityId seid) const;
    [[nodiscard]] std::size_t elementCount(base::EntityId seid) const;
    // Every interior point and every element of the list superelements, by superelement then ID.
    [[nodiscard]] const std::vector<InteriorPoint>& interiorPoints() const noexcept { return m_interiorPoints; }
    [[nodiscard]] const std::vector<PartitionElement>& elements() const noexcept { return m_elements; }
    // What the partition could not decide (an element on interior points of two superelements).
    [[nodiscard]] const std::vector<std::string>& problems() const noexcept { return m_problems; }

private:
    std::vector<BoundaryLink> m_links;
    std::map<SuperelementPoint, std::string> m_dofs;
    std::map<base::EntityId, std::size_t> m_interiorCounts;
    std::map<base::EntityId, std::size_t> m_elementCounts;
    std::vector<InteriorPoint> m_interiorPoints;
    std::vector<PartitionElement> m_elements;
    std::vector<std::string> m_problems;
};

} // namespace oor::analysis
