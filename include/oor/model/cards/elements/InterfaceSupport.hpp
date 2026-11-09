#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

// What the interface elements share (the acoustic infinite elements CACINF3 and CACINF4, the axisymmetric line CAXISYM,
// the 2-D bush CBUSH2D and the cohesive elements CIFHEX, CIFPENT, CIFQDX and CIFQUAD): the rule of an element ID and
// ONE kind of value, a row of connection points that starts at field 3 (after EID and PID), whose first `required`
// points must be given and whose others may be blank. No helper describes the fields of a card: each card still says
// how many points it has, which of them are required and what its other fields are.
namespace oor::model
{

// The format reference gives every one of these elements an ID of 1 to 99,999,999.
inline constexpr base::EntityId INTERFACE_ELEMENT_ID_LIMIT = 100000000;

// The field names of the connection points, in the order of the cards (the names of a diagnostic are literals).
inline constexpr std::array<const char*, 20> INTERFACE_NODE_NAMES{
    "node1",  "node2",  "node3",  "node4",  "node5",  "node6",  "node7",  "node8",  "node9",  "node10",
    "node11", "node12", "node13", "node14", "node15", "node16", "node17", "node18", "node19", "node20"};

// The problem with an element ID, if there is one.
[[nodiscard]] inline std::optional<CardProblem> checkInterfaceElementId(base::EntityId id)
{
    if (id <= 0 || id >= INTERFACE_ELEMENT_ID_LIMIT)
        return CardProblem{"card.bad-id", "the element ID must be an integer from 1 to 99999999", "id"};
    return std::nullopt;
}

// The names of the points of a card that does not call them G1, G2, ... (the GA and GB of a CBUSH2D): at least as many
// literals as the card has points.
using InterfaceNodeNames = std::span<const char* const>;

// The problem with the connection points of an element, if there is one: the first `required` are positive IDs, the
// others are positive or 0 (absent), and with `distinct` no ID occurs twice (format reference: "Unique"). A problem
// names the field of the point in `names`.
template <std::size_t N>
[[nodiscard]] std::optional<CardProblem> checkInterfaceNodes(const std::array<base::EntityId, N>& nodes,
                                                             std::size_t required, bool distinct,
                                                             InterfaceNodeNames names = INTERFACE_NODE_NAMES)
{
    for (std::size_t index = 0; index < N; ++index)
    {
        if (index < required && nodes[index] <= 0)
            return CardProblem{"card.bad-id", "the grid point IDs of the required points must be positive integers",
                               names[index]};
        if (nodes[index] < 0)
            return CardProblem{"card.bad-id", "a grid point ID must not be negative", names[index]};
    }
    if (!distinct)
        return std::nullopt;
    for (std::size_t first = 0; first < N; ++first)
    {
        if (nodes[first] == 0)
            continue;
        for (std::size_t second = first + 1; second < N; ++second)
        {
            if (nodes[first] == nodes[second])
                return CardProblem{"card.duplicate-node", "the grid points of an element must all be different",
                                   names[first]};
        }
    }
    return std::nullopt;
}

// Reads the points from field 3 on: a required one is an error when blank, an optional one reads 0 when blank.
template <std::size_t N>
void readInterfaceNodes(FieldReader& reader, std::array<base::EntityId, N>& nodes, std::size_t required,
                        InterfaceNodeNames names = INTERFACE_NODE_NAMES)
{
    for (std::size_t index = 0; index < N; ++index)
    {
        const int field = 3 + static_cast<int>(index);
        nodes[index] =
            index < required ? reader.requiredInteger(field, names[index]) : reader.integer(field, names[index], 0);
    }
}

// Writes the points from field 3 on; an absent optional point is a blank field.
template <std::size_t N>
void writeInterfaceNodes(FieldWriter& writer, const std::array<base::EntityId, N>& nodes, std::size_t required)
{
    for (std::size_t index = 0; index < N; ++index)
    {
        if (index < required || nodes[index] != 0)
            writer.integer(3 + static_cast<int>(index), nodes[index]);
    }
}

// Visits the points of a row of the column `column` as grid points and stores what the visitor returns; an absent
// point (0) is not visited.
template <std::size_t N>
void visitInterfaceNodes(ReferenceVisitor& visitor, Column<std::array<base::EntityId, N>>& column, std::uint32_t row)
{
    std::array<base::EntityId, N> nodes = column.get(row);
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        if (node <= 0)
            continue;
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        column.set(row, nodes);
}

} // namespace oor::model
