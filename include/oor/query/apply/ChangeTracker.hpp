#pragma once

#include "oor/query/apply/CellValue.hpp"
#include "oor/query/apply/ColumnEdits.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace oor::query
{

// One change: a cell of the table, the value it had in the table the edits started from, and the value it has now.
struct CellChange
{
    std::size_t row = 0;
    std::size_t column = 0;
    CellValue oldValue;
    CellValue newValue;
};

// The per-cell changes made to a table: one `ColumnEdits` for each column that was edited, made when the column is
// first written. The table it tracks is never modified; reading a cell gives the changed value when there is one.
class ChangeTracker
{
public:
    explicit ChangeTracker(std::shared_ptr<const QueryTable> table) : m_table(std::move(table)) {}

    [[nodiscard]] const std::shared_ptr<const QueryTable>& table() const noexcept { return m_table; }

    // The edits of a column, made on the first call.
    [[nodiscard]] ColumnEdits& edits(std::size_t column);
    // The edits of a column, or null when it was never written.
    [[nodiscard]] const ColumnEdits* find(std::size_t column) const noexcept;

    [[nodiscard]] std::size_t changeCount() const noexcept;
    [[nodiscard]] bool empty() const noexcept { return changeCount() == 0; }
    // The columns that have changed cells, in the order of the table.
    [[nodiscard]] std::vector<std::size_t> changedColumns() const;
    [[nodiscard]] bool isChanged(std::size_t column, std::size_t row) const noexcept;

    // The value of a cell now.
    [[nodiscard]] CellValue value(std::size_t column, std::size_t row) const;
    // The value of a cell in the table the changes started from.
    [[nodiscard]] CellValue originalValue(std::size_t column, std::size_t row) const;

    // The changes, by row and then by column; at most `limit` of them (0: all).
    [[nodiscard]] std::vector<CellChange> changes(std::size_t limit = 0) const;

    void clear(std::size_t column);
    void clear();

    // Replaces the table the changes are relative to (after they were written elsewhere); every change is dropped.
    void rebase(std::shared_ptr<const QueryTable> table);

private:
    std::shared_ptr<const QueryTable> m_table;
    std::vector<std::unique_ptr<ColumnEdits>> m_edits; // by column, null until written
};

} // namespace oor::query
