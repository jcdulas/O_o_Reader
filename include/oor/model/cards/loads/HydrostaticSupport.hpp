#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

// What HYDROC and HYDROS have in common: the field that says where the z direction of the fluid is measured from. One
// kind of value, one helper; nothing here describes the fields of a card, each card still lays out, reads, writes and
// checks its own fields by hand.
//
// The CID/GRD field holds one number whose sign is the switch (format reference): zero or above is the ID of a
// coordinate system (0 is the basic one) and its z axis is the direction; below zero is minus the ID of a grid point,
// and the z axis of the system of that point's CD field is the direction. The card keeps the number as written, sign
// included, so that the switch stays on the card; the helpers say what the number names.
namespace oor::model
{

// The ID of the coordinate system the field names, or 0 when it names the basic system or a grid point.
[[nodiscard]] constexpr base::EntityId hydrostaticSystemOf(base::EntityId frameOrNode) noexcept
{
    return frameOrNode > 0 ? frameOrNode : 0;
}

// The ID of the grid point the field names, or 0 when it names a coordinate system.
[[nodiscard]] constexpr base::EntityId hydrostaticGridOf(base::EntityId frameOrNode) noexcept
{
    return frameOrNode < 0 ? -frameOrNode : 0;
}

// The problem with a punch or override option (PCH, PLD4, OVRD), if there is one: each is an integer from 0 up to
// `highest` (3 for PCH, 1 for the other two). `field` is the
// neutral field name of the problem; the message names it by its keyword.
[[nodiscard]] inline std::optional<CardProblem> checkHydrostaticOption(std::int32_t value, const char* field,
                                                                       std::int32_t highest)
{
    if (value < 0 || value > highest)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be between 0 and " + std::to_string(highest),
                           field};
    return std::nullopt;
}

// The problem with the field, if there is one. Every integer has a meaning, except the smallest 32-bit one, whose
// negative is not an ID (and cannot be negated). The field holds either a system or a grid point: `frameId/nodeAlt`.
[[nodiscard]] inline std::optional<CardProblem> checkHydrostaticReference(base::EntityId frameOrNode)
{
    if (frameOrNode == std::numeric_limits<base::EntityId>::min())
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " is not a valid coordinate system or grid point ID",
                           "frameId/nodeAlt"};
    return std::nullopt;
}

// Visits what the field names, and stores what the visitor returns: a coordinate system when the number is above zero
// (the basic system, 0, is not a reference), a grid point when it is below zero. The sign stays on the field.
inline void visitHydrostaticReference(ReferenceVisitor& visitor, std::string_view field, base::EntityId& frameOrNode)
{
    if (frameOrNode > 0)
        frameOrNode = visitor.visit(field, ReferenceTarget::Coordinate, frameOrNode);
    else if (frameOrNode < 0)
        frameOrNode = -visitor.visit(field, ReferenceTarget::Node, -frameOrNode);
}

} // namespace oor::model
