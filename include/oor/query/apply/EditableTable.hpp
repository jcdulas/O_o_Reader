#pragma once

#include "oor/query/apply/CellValue.hpp"
#include "oor/query/apply/ChangeTracker.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// What the layer that owns the field setters of the cards tells the table: whether the field of a card type can be
// written. The cards are written through the setters of their Python classes (there is no list of the fields of a card
// in the model layer), so the answer comes from there.
class EditPolicy
{
public:
    virtual ~EditPolicy() = default;
    // Empty when the field of this card type has a setter; else the reason it has none. `sampleKey` is the key of a
    // card of the type (the setters are found on an object of the card's class).
    [[nodiscard]] virtual std::string fieldProblem(std::uint16_t cardType, const std::string& field,
                                                   std::uint32_t sampleKey) const = 0;
};

// Whether a column of an editable table can be written, and if not, why.
struct ColumnEditability
{
    bool editable = false;
    // The column is a position in the basic system of grid points (x, y, z, xyzGlobal); `apply` converts it back to
    // the CP system of each node.
    bool position = false;
    std::string reason; // why not, when it is not editable
    // Card types of the table whose cards cannot take this column (an editable column of a table of several card
    // types).
    std::vector<std::uint16_t> rejectedTypes;
};

struct UpdateOptions
{
    // The column that holds the keys, in the table and in the data; empty: the key ID column of the table's cards, else
    // `cardKey`.
    std::string on;
    // True: rows of the data that match no row of the table are left out; false: they are an error.
    bool ignoreMissing = false;
    // Only these columns of the data are taken (naming a read-only one is an error); empty: every column that is in the
    // table and is editable.
    std::vector<std::string> columns;
};

struct UpdateResult
{
    std::size_t matchedKeys = 0;  // rows of the data that matched rows of the table
    std::size_t matchedRows = 0;  // rows of the table those keys stand for (a key can repeat in a joined table)
    std::size_t cellsChanged = 0; // cells that differed and were written
    std::vector<std::int64_t> missingKeys;
    std::vector<std::string> columnsUpdated;
    std::map<std::string, std::string> columnsIgnored; // name -> why it was not taken
};

// A table with changes made to it: the table as it was made, never modified (a read-only Arrow snapshot), and the
// per-cell changes (`ChangeTracker`). Editable columns are the stored columns of the cards of the source, and the
// basic-system positions of grid points; every other column is read-only and writing one raises ReadOnlyColumnError at
// once, naming the column and why. Reading a cell gives the changed value; `current()` is the table with the changes
// put in (new buffers for the changed columns, shared buffers for the others), kept until the next change.
class EditableTable
{
public:
    explicit EditableTable(std::shared_ptr<const QueryTable> base, std::shared_ptr<const EditPolicy> policy = nullptr);

    [[nodiscard]] const std::shared_ptr<const QueryTable>& base() const noexcept { return m_tracker.table(); }
    [[nodiscard]] std::size_t rowCount() const noexcept { return static_cast<std::size_t>(base()->rowCount()); }
    [[nodiscard]] std::size_t columnCount() const noexcept { return base()->columnCount(); }
    [[nodiscard]] const ChangeTracker& tracker() const noexcept { return m_tracker; }

    // The index of a column; throws QueryError, naming the columns that exist, for a name the table has not.
    [[nodiscard]] std::size_t columnIndex(std::string_view name) const;

    [[nodiscard]] const ColumnEditability& editability(std::size_t column) const;
    [[nodiscard]] const ColumnEditability& editability(std::string_view name) const
    {
        return editability(columnIndex(name));
    }

    // --- changes ------------------------------------------------------------------------------------------------

    [[nodiscard]] std::size_t changeCount() const noexcept { return m_tracker.changeCount(); }
    [[nodiscard]] bool hasChanges() const noexcept { return changeCount() > 0; }

    // Throws ReadOnlyColumnError, naming the column and why, when the column cannot be edited at all.
    void requireEditable(std::size_t column) const;

    // Writes cells of an editable column. Each call writes all its cells or none: a read-only column raises
    // ReadOnlyColumnError, a value that does not fit the column raises QueryTypeError, a row outside the table raises
    // QueryError.
    void setCell(std::size_t column, std::size_t row, const CellValue& value);
    // One value for all the rows.
    void setBroadcast(std::size_t column, std::span<const std::uint32_t> rows, const CellValue& value);
    // One value for each row (or one for all of them) of a column of numbers.
    void setReals(std::size_t column, std::span<const std::uint32_t> rows, std::span<const double> values);
    void setIntegers(std::size_t column, std::span<const std::uint32_t> rows, std::span<const std::int64_t> values);
    // One value for each row.
    void setValues(std::size_t column, std::span<const std::uint32_t> rows, std::span<const CellValue> values);

    // Takes cells back to the value they had in the table: a column, or some rows of it, or everything.
    void reset();
    void reset(std::size_t column);
    void reset(std::size_t column, std::span<const std::uint32_t> rows);

    [[nodiscard]] CellValue value(std::size_t column, std::size_t row) const { return m_tracker.value(column, row); }
    [[nodiscard]] CellValue originalValue(std::size_t column, std::size_t row) const
    {
        return m_tracker.originalValue(column, row);
    }
    [[nodiscard]] std::vector<CellChange> changes(std::size_t limit = 0) const { return m_tracker.changes(limit); }

    // The column with the changes put in.
    [[nodiscard]] QueryColumn currentColumn(std::size_t column) const;
    // The table with the changes put in (the table itself when nothing changed).
    [[nodiscard]] std::shared_ptr<const QueryTable> current() const;

    // Takes the changes from a table of data: rows are matched to the rows of this table by key, and every cell that
    // differs is written. See UpdateOptions. Nothing is written when any part of it fails.
    UpdateResult update(const QueryTable& data, const UpdateOptions& options = {});

    // The changes were written to the model: the table with the changes put in becomes the table the changes are
    // relative to, and the changes are dropped.
    void commit();

    // --- the cards behind the rows ---------------------------------------------------------------------------------

    [[nodiscard]] bool hasCards() const noexcept { return base()->origin().isPresent(); }
    [[nodiscard]] std::uint16_t cardTypeAt(std::size_t row) const { return (*base()->origin().types)[row]; }
    [[nodiscard]] std::uint32_t cardKeyAt(std::size_t row) const { return (*base()->origin().keys)[row]; }

    // The names of the key ID columns of the card types of the table (the first stored column of a card type that has
    // IDs): `elementId` for elements, `nodeId` for grid points.
    [[nodiscard]] std::vector<std::string> keyFields() const;

private:
    void checkWritable(std::size_t column, std::span<const std::uint32_t> rows) const;
    [[nodiscard]] ColumnEditability computeEditability(std::size_t column) const;
    void mirrorPositions(std::size_t column, std::span<const std::uint32_t> rows);
    [[nodiscard]] std::optional<std::size_t> positionColumn(std::string_view field) const;
    void invalidate(std::size_t column);
    [[nodiscard]] std::map<std::uint16_t, std::uint32_t> typesPresent() const;

    ChangeTracker m_tracker;
    std::shared_ptr<const EditPolicy> m_policy;
    mutable std::vector<std::optional<ColumnEditability>> m_editability;
    mutable std::vector<std::optional<QueryColumn>> m_currentColumns;
    mutable std::shared_ptr<const QueryTable> m_current;
    mutable std::optional<std::map<std::uint16_t, std::uint32_t>> m_types; // card type -> the key of a card of it
};

// True for the fields that are positions in the basic system: x, y, z, xyzGlobal.
[[nodiscard]] bool isPositionField(std::string_view field) noexcept;

} // namespace oor::query
