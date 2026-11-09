#pragma once

#include "oor/analysis/mass/MassProperties.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"

#include <cstdint>
#include <map>
#include <vector>

namespace oor::query
{

// What a group of elements adds up to: how many elements contribute, the mass of the material (structural) and the
// non-structural mass, and the running sums that give the centre of gravity and the inertia.
struct GroupTotals
{
    std::int64_t count = 0;
    double structural = 0.0;
    double nonStructural = 0.0;
    analysis::MassSums sums;

    [[nodiscard]] double mass() const noexcept { return structural + nonStructural; }
};

// Adds an element to a group: `structuralShare` of its structural mass (1 for all of it) and, when `withNonStructural`,
// its non-structural mass; both are at the centre of the element. The inertia of the element about its own centre goes
// with the structural part when `withInertia`. An element whose mass cannot be computed (NaN) adds nothing; `counted`
// says whether it counts in the element count of the group.
void addElementMass(GroupTotals& group, const ElementMassData& masses, std::size_t index, double structuralShare,
                    bool withNonStructural, bool withInertia, bool counted = true);

// The groups of the structural mass by material: each material takes the structural mass of the elements that use it (a
// laminate shares its mass between the materials of its plies in proportion to density times thickness), the key 0
// takes the non-structural mass of every element and the mass of the elements that have no material.
[[nodiscard]] std::map<std::int32_t, GroupTotals> materialGroups(ElementData& data);

// The rows of a mass table in the default order: by decreasing mass, ties in the order they were made.
[[nodiscard]] std::vector<std::size_t> orderByMass(const std::vector<GroupTotals>& groups);

// The columns every mass table ends with, for the groups in `order`: elementCount, structuralMass,
// nonstructuralMass, mass, cx cy cz (the centre of gravity; null for a group with no mass).
void addMassColumns(TableBuilder& builder, const std::vector<GroupTotals>& groups,
                    const std::vector<std::size_t>& order);

} // namespace oor::query
