#pragma once

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <cstdint>
#include <set>
#include <vector>

// The degrees of freedom the constraint checks compare: which ones the rigid elements and the MPC equations make
// dependent, and which ones the single point constraints fix.

namespace oor::analysis
{

// A degree of freedom (a grid point and a component from 1 to 6, 0 for a scalar point) as one number.
[[nodiscard]] constexpr std::uint64_t dofKey(base::EntityId node, unsigned component) noexcept
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(node)) << 3U) | component;
}
[[nodiscard]] constexpr base::EntityId nodeOfDof(std::uint64_t key) noexcept
{
    return static_cast<base::EntityId>(key >> 3U);
}
[[nodiscard]] constexpr unsigned componentOfDof(std::uint64_t key) noexcept
{
    return static_cast<unsigned>(key & 7U);
}

// A degree of freedom and the card that puts it in a set.
struct DofUse
{
    std::uint64_t key = 0;
    model::CardLocation where;
};

// The degrees of freedom a component number names at a node: the digits 1 to 6 of 123, or the one scalar degree of
// freedom (0) when the number is 0.
void appendDofs(base::EntityId node, std::int32_t components, model::CardLocation where, std::vector<DofUse>& out);

// The degrees of freedom that the rigid elements make dependent: RBE2 (the GM at the components CM), RBE3 (the
// reference grid, or the UM list when there is one), RBAR and RROD (the CMA and CMB ends), RBE1 (the dependent list).
[[nodiscard]] std::vector<DofUse> rigidDependentDofs(const model::Model& model);

// A rigid element that makes a degree of freedom depend on another: every dependent degree of freedom of the element on
// every independent one (RBE2: the GM at CM on the six of GN; RBE3: the reference degrees of freedom, or the UM list,
// on the degrees of freedom of the weighted groups; RBE1 and RBAR: the dependent set on the independent set; RROD: the
// dependent translation on the other translations of both ends).
struct DofLink
{
    std::uint64_t dependent = 0;
    std::uint64_t independent = 0;
    model::CardLocation where;
};
[[nodiscard]] std::vector<DofLink> rigidDofLinks(const model::Model& model);

// The dependent degree of freedom (the first term) of every MPC card of the sets.
[[nodiscard]] std::vector<DofUse> mpcDependentDofs(const model::Model& model, const std::vector<base::EntityId>& sets);

// The degrees of freedom fixed by the SPC and SPC1 cards of the sets, and by the PS field of the grid points. The
// nodes of an SPC1 range are the grid points that exist in it (`nodes` is the sorted list of the node IDs).
[[nodiscard]] std::vector<DofUse> spcDofs(const model::Model& model, const std::vector<base::EntityId>& sets,
                                          const std::vector<base::EntityId>& nodes);

} // namespace oor::analysis
