#pragma once

#include "oor/query/table/ModelLink.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// Where the values of a column come from. The change tracking needs to know it: a stored column is a field
// of the cards of the source and can be edited, every other kind is read-only.
enum class ColumnOrigin : std::uint8_t
{
    Stored,   // a column of the card table of the source
    Computed, // a quantity computed from the model: geometry, mass, quality, adjacency counts, resolved rows
    Joined,   // a column of the cards a join followed to (a property, a material, a grid point)
    Derived,  // the result of an expression, an aggregate, or a table made from arrays
    // A computed quantity of an earlier query that a query on its table holds as plain values: the engine reads it like
    // a stored column, but it is not a field of the cards and stays read-only (the basic positions x, y, z excepted).
    Materialised
};

// What a table knows about one of its columns, next to the data.
struct ColumnInfo
{
    std::string name;
    ColumnOrigin origin = ColumnOrigin::Derived;
    // The name of the field in the card (a stored column, or a column of a join with a prefix: the name without it).
    std::string field;
};

// The card each row of a table was made from, for a table that has rows of cards (a source, possibly filtered, sorted,
// limited or joined, and not grouped): the card type index and the card key. A key never changes whatever happens to
// the ID of a card, so the rows still find their cards after a renumbering. `apply` writes
// edits through it, to the model the table was made from, which it finds through `link` (the table does not keep the
// model alive: `apply` on a table whose model is gone asks for the model to write to).
struct RowOrigin
{
    std::shared_ptr<const std::vector<std::uint16_t>> types;
    std::shared_ptr<const std::vector<std::uint32_t>> keys;
    std::shared_ptr<const ModelLink> link;
    // The model the rows were made from, or null when there is no link or the model is gone.
    [[nodiscard]] std::shared_ptr<model::Model> model() const { return link ? link->resolve() : nullptr; }
    [[nodiscard]] bool isPresent() const noexcept { return types != nullptr && keys != nullptr; }
};

// What a run did, for the tests and `explain`: the computed columns that were really evaluated, and how many rows each
// stage of the plan saw.
struct QueryReport
{
    std::vector<std::string> computedColumns; // in the order they were first evaluated
    std::int64_t sourceRows = 0;
    std::int64_t rowsAfterFilter = 0;
};

// Where the values of a result table came from (source resolution): one note for each way
// (provider) that produced rows, with the columns it filled and the number of rows. A value that a recomputation made
// is never presented as a stored one, and an approximate way says so.
struct SourceNote
{
    std::string provider;
    bool approximate = false;
    std::vector<std::string> columns;
    std::int64_t rows = 0;
    // The status of the result tables the rows were read from: "validated", "notValidated" or "experimental"; the
    // least trusted of them when there are several; empty when the rows were not read from a result table.
    std::string status;
};

// A table: named, typed columns of the same length, in the Arrow memory layout, owned by shared buffers. It is
// immutable: an operator makes a new table, sharing every buffer it did not change. A table built from the columns of a
// model shares the buffers of the model (copy on write there), so it is a snapshot that costs no copy.
class QueryTable
{
public:
    QueryTable() = default;

    // Adds a column; every column of a table has the same length. Throws QueryError on a repeated name or a wrong
    // length.
    void addColumn(std::string name, QueryColumn column, ColumnOrigin origin = ColumnOrigin::Derived,
                   std::string field = {});

    [[nodiscard]] std::size_t columnCount() const noexcept { return m_columns.size(); }
    [[nodiscard]] std::int64_t rowCount() const noexcept { return m_rows; }
    [[nodiscard]] const QueryColumn& column(std::size_t index) const { return m_columns[index]; }
    [[nodiscard]] const ColumnInfo& info(std::size_t index) const { return m_infos[index]; }
    [[nodiscard]] const std::string& name(std::size_t index) const { return m_infos[index].name; }
    [[nodiscard]] std::optional<std::size_t> find(std::string_view name) const noexcept;
    [[nodiscard]] bool has(std::string_view name) const noexcept { return find(name).has_value(); }
    // The column called `name`; throws QueryError (with the names that exist) when there is none.
    [[nodiscard]] const QueryColumn& column(std::string_view name) const;
    [[nodiscard]] std::vector<std::string> columnNames() const;

    [[nodiscard]] const RowOrigin& origin() const noexcept { return m_origin; }
    void setOrigin(RowOrigin origin) { m_origin = std::move(origin); }
    // The providers of the values of a result table (empty for a table of the model).
    [[nodiscard]] const std::vector<SourceNote>& sources() const noexcept { return m_sources; }
    void setSources(std::vector<SourceNote> sources) { m_sources = std::move(sources); }
    [[nodiscard]] const QueryReport& report() const noexcept { return m_report; }
    void setReport(QueryReport report) { m_report = std::move(report); }

    // The version of the data contract written in the metadata of the Arrow schema; 0 writes nothing.
    [[nodiscard]] int contractVersion() const noexcept { return m_contractVersion; }
    void setContractVersion(int version) noexcept { m_contractVersion = version; }

private:
    std::vector<ColumnInfo> m_infos;
    std::vector<QueryColumn> m_columns;
    std::int64_t m_rows = 0;
    RowOrigin m_origin;
    std::vector<SourceNote> m_sources;
    QueryReport m_report;
    int m_contractVersion = 0;
};

} // namespace oor::query
