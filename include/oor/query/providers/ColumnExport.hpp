#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"
#include "oor/query/providers/StoredColumnCatalogue.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The columns of the cards as the query engine reads them: every card type registers its stored columns ("nodeId",
// "positionFrameId", "xyz" of a GRID) in the StoredColumnCatalogue, each with a getter that turns a column of the model
// into a QueryColumn sharing its buffer (no copy). The registrations are in src/query/providers/columns/ (one file a
// family of cards); the helpers they use are here.
namespace oor::query::column_export
{

namespace detail
{

template <typename T>
struct ElementTraits
{
    using Scalar = T;
    static constexpr std::size_t WIDTH = 1;
};
template <typename T, std::size_t N>
struct ElementTraits<std::array<T, N>>
{
    using Scalar = T;
    static constexpr std::size_t WIDTH = N;
};

// The values of a buffer of scalars or of fixed-size arrays of scalars as a query column, sharing the buffer.
template <typename T>
QueryColumn queryColumnOf(std::shared_ptr<const std::vector<T>> buffer)
{
    using Traits = ElementTraits<T>;
    if constexpr (Traits::WIDTH == 1)
        return QueryColumn::primitiveShared<T>(std::move(buffer));
    else
        return QueryColumn::fixedListShared<typename Traits::Scalar, Traits::WIDTH>(std::move(buffer));
}

} // namespace detail

// A column of one value (or one fixed-size array) a card.
template <typename T>
QueryColumn exportColumn(model::Column<T>& column)
{
    return detail::queryColumnOf<T>(column.share());
}

// A column of a variable number of values a card: a list column.
template <typename T>
QueryColumn exportVariableColumn(model::VariableLengthColumn<T>& column)
{
    column.canonicalise();
    const std::vector<std::uint64_t> raw = column.offsets();
    std::vector<std::int64_t> offsets(raw.begin(), raw.end());
    const auto length = static_cast<std::int64_t>(column.size());
    return QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), detail::queryColumnOf<T>(column.shareValues()),
                             length);
}

// A column of one text a card.
inline QueryColumn exportTextColumn(model::Column<std::string>& column)
{
    std::vector<std::string> texts;
    texts.reserve(column.size());
    for (std::size_t row = 0; row < column.size(); ++row)
        texts.push_back(column.get(row));
    return QueryColumn::utf8(texts);
}

// A column of a list of texts a card.
inline QueryColumn exportTextListColumn(const std::vector<std::vector<std::string>>& lists)
{
    std::vector<std::int64_t> offsets{0};
    std::vector<std::string> texts;
    for (const std::vector<std::string>& list : lists)
    {
        texts.insert(texts.end(), list.begin(), list.end());
        offsets.push_back(static_cast<std::int64_t>(texts.size()));
    }
    return QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), QueryColumn::utf8(texts),
                             static_cast<std::int64_t>(lists.size()));
}

// A variable-length column of texts: a list column of strings.
inline QueryColumn exportVariableTextColumn(const model::VariableLengthColumn<std::string>& column)
{
    std::vector<std::vector<std::string>> lists;
    lists.reserve(column.size());
    for (std::size_t row = 0; row < column.size(); ++row)
    {
        const std::span<const std::string> texts = column.get(row);
        lists.emplace_back(texts.begin(), texts.end());
    }
    return exportTextListColumn(lists);
}

// The column of one value of each card of a table whose cards are kept whole: `valueOf(card)` for each row.
template <typename TableT, typename ValueOf>
QueryColumn exportCardValues(TableT& table, ValueOf valueOf)
{
    using Value = std::decay_t<decltype(valueOf(table.card(0)))>;
    model::Column<Value> column;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
        column.push(valueOf(table.card(row)));
    if constexpr (std::is_same_v<Value, std::string>)
        return exportTextColumn(column);
    else
        return exportColumn(column);
}

// The column of a list of numbers of each card of a table whose cards are kept whole.
template <typename TableT, typename ListOf>
QueryColumn exportCardLists(TableT& table, ListOf listOf)
{
    using Value = typename std::decay_t<decltype(listOf(table.card(0)))>::value_type;
    model::VariableLengthColumn<Value> column;
    for (std::uint32_t row = 0; row < table.rows().rowCount(); ++row)
    {
        const auto values = listOf(table.card(row));
        column.push(std::span<const Value>(values.data(), values.size()));
    }
    return exportVariableColumn(column);
}

// Registers a column: `getter` takes the model (as a shared pointer that does not own it) and returns the column.
template <typename Getter>
void registerColumn(StoredColumnCatalogue& catalogue, std::uint16_t type, std::string name, Getter getter)
{
    catalogue.add(type, std::move(name),
                  [getter](model::Model& model)
                  {
                      const std::shared_ptr<model::Model> borrowed(std::shared_ptr<void>(), &model);
                      return getter(borrowed);
                  });
}

} // namespace oor::query::column_export
