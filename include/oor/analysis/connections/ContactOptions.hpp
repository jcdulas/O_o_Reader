#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// What contact between bodies does to the components.
enum class ContactMode : std::uint8_t
{
    Merge,      // bodies in contact belong to one component (the default)
    Connection, // the contact is an assembly-level connection, like a fastener, with its node pairs
    Ignore      // contact plays no part
};

// "merge", "connection" or "ignore".
[[nodiscard]] std::string_view contactModeName(ContactMode mode) noexcept;
// The mode of a name; nothing for another text.
[[nodiscard]] std::optional<ContactMode> contactModeOf(std::string_view name) noexcept;

// How the node pairs of a contact are selected (glued contact for a linear solver).
enum class ContactRule : std::uint8_t
{
    // The components' rule: the distance of the pair, else the caller's, else 5 % of the median
    // edge; a grid point between the skins of a shell meets it; a pair that finds nothing from its secondary body is
    // searched from its primary body.
    Components,
    // The selection the solver makes for a glued pair: its distance tolerance, the
    // BIAS of the pair, the double-order search of ISEARCH 0 and the grid points it leaves out.
    Solver
};

// "components" or "solver".
[[nodiscard]] std::string_view contactRuleName(ContactRule rule) noexcept;
// The rule of a name; nothing for another text.
[[nodiscard]] std::optional<ContactRule> contactRuleOf(std::string_view name) noexcept;

// The contact options of the component queries: the mode; the distance below which a grid point of the secondary
// surface meets the primary surface when the pair gives none (nothing: 5 % of the median edge length of the secondary
// surface; the components' rule only); and the rule that selects the node pairs. With ContactRule::Solver the
// components are built on the node pairs the solver ties: in "connection" mode the interface points of a glued pair are
// then the points its glue equations use, so a condensation keeps every point an equation needs.
//
// `connections` are the elements the caller declares connections, their IDs sorted and distinct: each
// is part of no component whatever its type, a connection of the kind of its family when that family is a connection
// (a connector, a rigid element), else a connector; the components are those of the other elements by the same rule.
// Empty: the components of the connectivity alone, as before. They go with the contact options because every component
// query takes both and the cache keeps a set of components for each.
struct ContactOptions
{
    ContactMode mode = ContactMode::Merge;
    std::optional<double> tolerance;
    ContactRule rule = ContactRule::Components;
    std::vector<base::EntityId> connections{};

    [[nodiscard]] bool operator==(const ContactOptions&) const noexcept = default;
};

} // namespace oor::analysis
