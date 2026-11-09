#pragma once

#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/analysis/mass/NonStructuralMass.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/plan/QuerySource.hpp"
#include "oor/query/providers/StoredColumnCatalogue.hpp"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// A list of cards of a model: for card number `i`, its card type and its row in the table of that type (NO_ROW for "no
// card": a join that found nothing).
struct CardRows
{
    std::span<const std::uint16_t> types; // empty: every card has the type `single`
    std::span<const std::uint32_t> rows;
    std::uint16_t single = 0;
    // The cards are exactly the rows 0 .. n-1 of the table of `single`, in order: a stored column is then the column of
    // the table itself, shared without copy.
    bool wholeTable = false;

    [[nodiscard]] std::size_t count() const noexcept { return rows.size(); }
    [[nodiscard]] std::uint16_t typeAt(std::size_t index) const noexcept
    {
        return types.empty() ? single : types[index];
    }
};

// What a query run has computed so far, for all models: the number of times each family of computed columns was
// evaluated and over how many rows. A test hook (`oor_reader._query_statistics()`): a column that was not selected must
// not show up here.
struct ComputationCounters
{
    std::atomic<std::uint64_t> geometryCalls{0};
    std::atomic<std::uint64_t> massCalls{0};
    std::atomic<std::uint64_t> qualityCalls{0};
    std::atomic<std::uint64_t> adjacencyCalls{0};
    std::atomic<std::uint64_t> resolvedCalls{0};
    std::atomic<std::uint64_t> positionCalls{0};
    std::atomic<std::uint64_t> rowsComputed{0};
};
[[nodiscard]] ComputationCounters& computationCounters() noexcept;

// The columns that cards of some card types offer to a query, and how to read them for a list of cards.
//
// Stored columns are the columns of the card tables (StoredColumnCatalogue). For several card types they are the union
// of the names, in order of first appearance: a column that a type lacks is null for its cards, fixed lists of
// different widths become lists, and a column whose layouts cannot be put together (a number in one type, a text in
// another) is left out.
//
// The type column gives the name of the card type of each card, dictionary encoded (it is named `card` when a card has
// a field called `type`).
//
// Computed columns are, for element cards: geometry (area, volume, length, thickness, centerX/y/z, normalX/y/z), mass
// (mass, structuralMass, nonstructuralMass), quality (the 14 metrics of the mesh quality), adjacency counts
// (edgeNeighbourCount/node/face) and the resolved rows (nodesRows, nodeARow, node1Row, ..., propIdRow, propType: rows
// of the exported tables, ExportedRowOrder); for grid points, the basic positions (x, y, z, xyzGlobal) and
// elementCount. Each is evaluated only when asked for, for the cards asked for.
class CardColumns
{
public:
    CardColumns(std::shared_ptr<model::Model> model, std::vector<std::uint16_t> types);

    [[nodiscard]] const std::shared_ptr<model::Model>& model() const noexcept { return m_model; }
    [[nodiscard]] const std::vector<std::uint16_t>& types() const noexcept { return m_types; }

    // The columns in the order of a query that selects nothing: stored, type, then computed.
    [[nodiscard]] std::vector<ColumnDescription> columns() const;
    [[nodiscard]] bool has(std::string_view name) const;

    // The values of column `name` for `rows`, plus the columns computed with it at no cost.
    [[nodiscard]] std::vector<NamedColumn> fetch(std::string_view name, const CardRows& rows,
                                                 ExecutionContext& context) const;

private:
    struct Stored
    {
        std::string name;
        ColumnLayout layout;                     // after putting the types together
        std::vector<std::uint16_t> contributing; // the card types whose column goes with it: the others give nulls
    };

    [[nodiscard]] QueryColumn fetchStored(std::string_view name, const CardRows& rows, ExecutionContext& context) const;
    [[nodiscard]] QueryColumn typeColumn(const CardRows& rows) const;
    // The mass options of the model (the NSM set of the Case Control, resolved once).
    [[nodiscard]] analysis::MassOptions massOptions(ExecutionContext& context) const;

    std::shared_ptr<model::Model> m_model;
    std::vector<std::uint16_t> m_types;       // the card types that have cards (or the one type asked for)
    std::vector<std::uint16_t> m_layoutTypes; // the card types that give the columns: the same, or all of a category
    std::vector<std::size_t> m_tableRows;     // rows of each table (dead ones included): the offsets of the types
    std::vector<std::size_t> m_offsets;       // where the rows of each type start in a column of all the types
    std::vector<Stored> m_stored;
    std::string m_typeColumnName = "type";

    mutable std::mutex m_mutex;
    mutable std::map<std::string, QueryColumn> m_joined; // columns of several types put end to end
    mutable bool m_massResolved = false;
    mutable std::optional<analysis::NonStructuralMass> m_nonStructural;
};

} // namespace oor::query
