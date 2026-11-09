#pragma once

#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/query/providers/CardColumns.hpp"

#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace oor::query::detail
{

// The quantities of the model that a query can select as columns, in families: a family is evaluated by one call to the
// analysis layer, which gives all its columns at once (or the one asked for, when the others cost more).
enum class Family : std::uint8_t
{
    Geometry,         // area, volume, length, thickness, centerX/y/z, normalX/y/z
    Mass,             // mass, structuralMass, nonstructuralMass
    Quality,          // the 14 metrics
    Adjacency,        // edgeNeighbourCount, nodeNeighbourCount, faceNeighbourCount
    ResolvedNodes,    // nodesRows
    ResolvedPoint,    // nodeARow, nodeBRow, node1Row, ...: the row of one grid point field
    ResolvedProperty, // propIdRow, propType
    NodePosition,     // x, y, z, xyzGlobal
    NodeCount         // elementCount
};

[[nodiscard]] std::optional<Family> familyOf(std::string_view column);

// The computed columns offered for cards of these types: the element families when every type is an element, the grid
// families when the only type is GRID, none otherwise. `hasStored(type, name)` says whether a type has a stored column
// (the resolved rows need `nodes`, `propId` and the single grid point fields).
[[nodiscard]] std::vector<std::string> computedNamesFor(
    std::span<const std::uint16_t> types, const std::function<bool(std::uint16_t, std::string_view)>& hasStored);

// Computes the column `column` (and the columns of its family that come with it) for the cards of `rows`.
[[nodiscard]] std::vector<NamedColumn> computeColumns(std::string_view column, model::Model& model,
                                                      const CardRows& rows, ExecutionContext& context,
                                                      const analysis::MassOptions& massOptions);

} // namespace oor::query::detail
