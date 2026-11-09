#pragma once

#include "oor/api/Deck.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// The card tables of a model: the columns of the cards themselves (every field of a ShellQuad4, ShellProp, IsoMat, ...)
// and the computed columns the cards have (the rows of their grid points and property: nodesRows, propIdRow, propType;
// the positions of the grid points: xyzGlobal), with the exact Arrow layout of the library. This is what a program that
// computes on the model (the O_o_Solver module) reads, where the named queries give views made for people.
namespace oor::api
{

// The columns a card table can give: the stored ones (the fields of the card, given by default) and the computed ones
// (given when asked for).
struct CardTableColumns
{
    std::vector<std::string> stored;
    std::vector<std::string> computed;
};

// What a card table is made of. `source` names the cards: a card type ("ShellQuad4", "ShellProp", "LanczosEigenMethod"),
// "nodes" for the grid points or "points" for the other points (SPOINT, EPOINT, ...). Throws QueryError for an unknown
// name.
[[nodiscard]] CardTableColumns cardTableColumns(const std::string& source);

// The table of the live cards of `source`, sorted by ID: the columns asked for, or every stored column when `columns`
// is empty. The rows are those the row-index columns of other card tables point at (nodesRows indexes the rows of the
// "nodes" table). Throws QueryError for an unknown source or column.
[[nodiscard]] std::shared_ptr<const query::QueryTable> cardTable(const Deck& deck, const std::string& source,
                                                                 const std::vector<std::string>& columns);

// The card types of the deck that have live cards, by category: "elements", "properties", "materials", "methods",
// "loads", "points", "nodes", "coordinates", "constraints" (the other categories are not listed). Sorted by name.
[[nodiscard]] std::vector<std::string> cardTypesOf(const Deck& deck, const std::string& category);

// For every grid point, in the rows of the "nodes" table, the rotation from its displacement system CD to the basic
// system: 9 values per grid point, a 3 x 3 matrix row-major whose columns are the directions 1, 2, 3 of CD in the basic
// system at the grid point (the identity for CD = 0).
[[nodiscard]] std::vector<double> displacementRotations(const Deck& deck);

// The version counters of the model: the structure version changes when cards are added or removed, the content
// version when any value changes. A program that keeps results of a model compares them to know whether they still
// hold.
struct ModelVersions
{
    std::uint64_t structure = 0;
    std::uint64_t content = 0;
};
[[nodiscard]] ModelVersions modelVersions(const Deck& deck);

} // namespace oor::api
