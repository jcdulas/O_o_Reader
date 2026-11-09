#include "oor/query/providers/ExportedRowOrder.hpp"

#include <algorithm>

namespace oor::query
{

std::vector<std::uint32_t> ExportedRowOrder::storageRows(const model::Model& model, std::uint16_t type)
{
    std::vector<std::uint32_t> rows;
    const model::CardTable* table = model.findTable(type);
    if (table == nullptr)
        return rows;
    const model::RowBook& book = table->rows();
    rows.reserve(book.aliveCount());
    for (std::uint32_t row = 0; row < book.rowCount(); ++row)
    {
        if (book.isAlive(row))
            rows.push_back(row);
    }
    // The storage rows of a table are in the order of their keys; a deck whose cards are in ID order needs no sort.
    const auto byId = [table](std::uint32_t left, std::uint32_t right)
    { return table->idAt(left) < table->idAt(right); };
    if (!std::is_sorted(rows.begin(), rows.end(), byId))
        std::stable_sort(rows.begin(), rows.end(), byId);
    return rows;
}

std::vector<std::int32_t> ExportedRowOrder::tableRows(const model::Model& model, std::uint16_t type)
{
    const model::CardTable* table = model.findTable(type);
    std::vector<std::int32_t> positions(table != nullptr ? table->rows().rowCount() : 0, -1);
    const std::vector<std::uint32_t> rows = storageRows(model, type);
    for (std::size_t position = 0; position < rows.size(); ++position)
        positions[rows[position]] = static_cast<std::int32_t>(position);
    return positions;
}

} // namespace oor::query
