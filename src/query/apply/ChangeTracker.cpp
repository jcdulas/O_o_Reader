#include "oor/query/apply/ChangeTracker.hpp"

#include <algorithm>

namespace oor::query
{

ColumnEdits& ChangeTracker::edits(std::size_t column)
{
    if (m_edits.size() < m_table->columnCount())
        m_edits.resize(m_table->columnCount());
    if (!m_edits[column])
        m_edits[column] = std::make_unique<ColumnEdits>(m_table->column(column));
    return *m_edits[column];
}

const ColumnEdits* ChangeTracker::find(std::size_t column) const noexcept
{
    return column < m_edits.size() ? m_edits[column].get() : nullptr;
}

std::size_t ChangeTracker::changeCount() const noexcept
{
    std::size_t total = 0;
    for (const std::unique_ptr<ColumnEdits>& edits : m_edits)
    {
        if (edits)
            total += edits->changedCount();
    }
    return total;
}

std::vector<std::size_t> ChangeTracker::changedColumns() const
{
    std::vector<std::size_t> columns;
    for (std::size_t column = 0; column < m_edits.size(); ++column)
    {
        if (m_edits[column] && m_edits[column]->changedCount() > 0)
            columns.push_back(column);
    }
    return columns;
}

bool ChangeTracker::isChanged(std::size_t column, std::size_t row) const noexcept
{
    const ColumnEdits* edits = find(column);
    return edits != nullptr && edits->isChanged(row);
}

CellValue ChangeTracker::value(std::size_t column, std::size_t row) const
{
    const ColumnEdits* edits = find(column);
    if (edits == nullptr)
        return cellAt(m_table->column(column), row);
    return edits->value(row, m_table->column(column));
}

CellValue ChangeTracker::originalValue(std::size_t column, std::size_t row) const
{
    return cellAt(m_table->column(column), row);
}

std::vector<CellChange> ChangeTracker::changes(std::size_t limit) const
{
    std::vector<CellChange> result;
    for (const std::size_t column : changedColumns())
    {
        for (const std::uint32_t row : m_edits[column]->changedRows())
        {
            CellChange change;
            change.row = row;
            change.column = column;
            result.push_back(std::move(change));
        }
    }
    std::sort(result.begin(), result.end(), [](const CellChange& left, const CellChange& right)
              { return left.row != right.row ? left.row < right.row : left.column < right.column; });
    if (limit > 0 && result.size() > limit)
        result.resize(limit);
    for (CellChange& change : result)
    {
        change.oldValue = originalValue(change.column, change.row);
        change.newValue = value(change.column, change.row);
    }
    return result;
}

void ChangeTracker::clear(std::size_t column)
{
    if (column < m_edits.size())
        m_edits[column].reset();
}

void ChangeTracker::clear()
{
    m_edits.clear();
}

void ChangeTracker::rebase(std::shared_ptr<const QueryTable> table)
{
    m_table = std::move(table);
    m_edits.clear();
}

} // namespace oor::query
