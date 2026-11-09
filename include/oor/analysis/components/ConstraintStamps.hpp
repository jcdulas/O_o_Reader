#pragma once

#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::analysis
{

// The constraint stamp of one unit for one SPC set.
struct ConstraintStamp
{
    std::int32_t unit = 0;
    std::optional<base::EntityId> spcSet; // nothing when the model has no SPC set: the permanent constraints alone
    std::size_t constrainedNodeCount = 0;  // the nodes of the unit with an entry
    std::uint64_t revision = 0;
};

// The constraint stamps of units: for each unit (a component or a group) and SPC set, a 64-bit
// hash of the constraints on its nodes, so that a reduction made with the constraints applied before reducing is known
// stale when they change, and only then.
//
// The entries of a set are the components of each node the SPC and SPC1 cards of the set give (SPCADD expanded; a THRU
// range of an SPC1 gives the grid points of the model in it) with the enforced value of an SPC, the enforced motions of
// the SPCD cards of every load set (an SPCD needs its degrees of freedom in an SPC set), and the
// permanent constraints of the grid points (GRID PS), which apply to every set. Each entry is hashed from its kind,
// node, components and value and the hashes are added: the stamp depends on the content, not on the cards that carry
// it, their order or the session. An edit outside the nodes of a unit leaves its stamp alone.
class ConstraintStamps
{
public:
    // `spcSets` empty: every set an SPC, SPC1 or SPCADD card defines or a subcase selects. `units` empty: every unit.
    // By unit, then set.
    [[nodiscard]] static std::vector<ConstraintStamp> compute(const model::Model& model, const UnitMembership& units,
                                                              std::vector<base::EntityId> spcSets = {},
                                                              const std::vector<std::int32_t>& unitsWanted = {});
};

} // namespace oor::analysis
