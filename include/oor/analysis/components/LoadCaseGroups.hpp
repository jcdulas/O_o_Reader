#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <vector>

namespace oor::analysis
{

// One load case (an analysis subcase of the Case Control) as it loads one component.
struct LoadCaseRow
{
    std::int32_t component = 0;
    std::int32_t group = 0; // 0: the load case applies no concentrated load to the component
    std::int64_t subcase = 0;
    std::int64_t loadSid = 0; // the LOAD set the subcase selects, 0 when it selects none (a SUBCOM has none)
    std::size_t loadedNodeCount = 0;
    // The load case has a pressure, an inertial or a thermal load on the component.
    bool isDistributed = false;
    // The load revision stamp: a hash of the load content applied to the component in this load case, combination
    // factors included.
    std::uint64_t loadRevision = 0;
};

// The load application nodes of one group of one component: the nodes that every load case of the group loads, and
// nothing else.
struct LoadGroupNodes
{
    std::int32_t component = 0;
    std::int32_t group = 0;
    std::vector<base::EntityId> nodes; // sorted
    std::vector<std::int64_t> subcases; // the load cases of the group, in the order of the Case Control
};

// The load cases of a model grouped, per component, by identical sets of load application nodes.
//
// For each analysis subcase (SUBCASE, SUBCOM, SYM, SYMCOM, REPCASE, in the order of the Case Control) the load set its
// LOAD command selects is expanded (LOAD combinations and their factors, LoadSets). A SUBCOM or SYMCOM loads what the
// subcases it combines load, each with its SUBSEQ or SYMSEQ factor: the combined subcases are the SUBCASE (SYM)
// sections that come before it, in order, and a factor of zero or a missing one applies no load. The application nodes
// of a component are the nodes of the component that carry a concentrated load (forces, moments, scalar loads) or an
// enforced motion (SPCD counts as a load). Pressures, inertial loads, temperatures and imposed deformations act on the
// elements: they set `isDistributed` and take part in the load revision stamp but add no node, so two load cases that
// load the same nodes are in one group whatever else they apply. Gravity, rotation and the default temperature act on
// every component. Group 0 holds the load cases with no application node on the component, which includes a case that
// has only distributed loads (`isDistributed` tells them from a case with no load at all). The other groups are
// numbered in the order their first load case appears.
//
// The same grouping is made for any units (UnitMembership: the groups of GroupSet); `component` is then
// the number of the unit, and a node of two units loads both.
class LoadCaseGroups
{
public:
    // `subcases` keeps the rows of these subcase numbers (empty: all); the group numbers do not depend on it.
    [[nodiscard]] static LoadCaseGroups compute(const model::Model& model, const ComponentModel& components,
                                                const std::vector<std::int64_t>& subcases = {});
    [[nodiscard]] static LoadCaseGroups compute(const model::Model& model, const UnitMembership& units,
                                                const std::vector<std::int64_t>& subcases = {});

    // The rows by component, group and subcase.
    [[nodiscard]] const std::vector<LoadCaseRow>& rows() const noexcept { return m_rows; }
    // The groups by component and group, group 0 of every component included.
    [[nodiscard]] const std::vector<LoadGroupNodes>& groups() const noexcept { return m_groups; }
    [[nodiscard]] const LoadGroupNodes* find(std::int32_t component, std::int32_t group) const noexcept;

private:
    std::vector<LoadCaseRow> m_rows;
    std::vector<LoadGroupNodes> m_groups;
};

} // namespace oor::analysis
