#pragma once

#include "oor/analysis/superelements/BoundaryLink.hpp"
#include "oor/analysis/superelements/SuperelementCatalogue.hpp"

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// The boundary points of a deck: the points that links join, directly or through others, are one boundary point (a
// point where three parts and the residual structure meet is one), numbered as the solver numbers them in its GPLS tables and
// its BOUNDARY SEQUENCE ASSIGNMENT TABLE: the primary superelements by ascending number, then the images (REPEAT,
// MIRROR, CSUPER) by ascending number, each its points by ascending ID; a boundary point not yet numbered takes the
// next number from 1. The residual structure never numbers one (its points are numbered through the superelements that
// join them).
class BoundaryPointNumbering
{
public:
    // `lonePoints`: boundary points that no link joins to anything (the EXTRN points of an external superelement that
    // nothing connects); they are boundary points of their own.
    BoundaryPointNumbering(const SuperelementCatalogue& catalogue, std::span<const BoundaryLink> links,
                           std::span<const SuperelementPoint> lonePoints = {});

    // The boundary points of a superelement (its own IDs), ascending.
    [[nodiscard]] std::vector<base::EntityId> pointsOf(base::EntityId seid) const;
    // The number of the boundary point a point is part of; 0 when it is none.
    [[nodiscard]] std::int32_t numberOf(const SuperelementPoint& point) const;
    // The point of superelement `seid` that is the same boundary point as `point` (the smallest ID when there are
    // several); nullopt when that superelement has none in it (the solver then creates a point there).
    [[nodiscard]] std::optional<base::EntityId> pointIn(const SuperelementPoint& point, base::EntityId seid) const;
    // The points that are the same boundary point as `point`, itself included, ascending.
    [[nodiscard]] std::vector<SuperelementPoint> membersOf(const SuperelementPoint& point) const;
    // How many boundary points the deck has.
    [[nodiscard]] std::int32_t count() const noexcept { return m_count; }

private:
    [[nodiscard]] std::optional<std::uint32_t> groupOf(const SuperelementPoint& point) const;

    std::map<SuperelementPoint, std::uint32_t> m_index;    // point -> its place in m_groupOf
    std::vector<std::uint32_t> m_groupOf;                  // the group of each point
    std::vector<std::vector<SuperelementPoint>> m_members; // the points of each group, ascending
    std::vector<std::int32_t> m_numbers;                   // the number of each group
    std::int32_t m_count = 0;
};

} // namespace oor::analysis
