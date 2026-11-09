#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// The type, fixed width and element type of a column, known without reading any data.
struct ColumnLayout
{
    ColumnType type = ColumnType::Int32;
    std::int32_t width = 0;                     // FixedList
    ColumnType elementType = ColumnType::Int32; // List and FixedList: the type of the values
    std::int32_t elementWidth = 0;              // a list of fixed lists: the width of the inner ones
    [[nodiscard]] bool operator==(const ColumnLayout&) const noexcept = default;
    [[nodiscard]] static ColumnLayout of(const QueryColumn& column);
};

// The columns each card type exports, as the query engine reads them. A card type has no list of its fields that a
// generic reader could use: each card type registers its columns (src/query/providers/columns/, one file a family of
// cards), each with a function that returns the column as a QueryColumn sharing the buffer of the model (no copy; the
// model clones the buffer on its next edit of that column, so the table is a snapshot). The order of the
// registration is the order of the columns of the card.
class StoredColumnCatalogue
{
public:
    using Fetcher = std::function<QueryColumn(model::Model&)>;

    [[nodiscard]] static StoredColumnCatalogue& instance();

    void add(std::uint16_t type, std::string name, Fetcher fetcher);

    // The names of the columns of a card type, in the order of the card.
    [[nodiscard]] std::vector<std::string> names(std::uint16_t type) const;
    [[nodiscard]] bool contains(std::uint16_t type, std::string_view name) const;
    // The column of the live table of a type (the rows are the rows of the table, dead ones included). Throws
    // QueryError for a column the type does not have.
    [[nodiscard]] QueryColumn fetch(model::Model& model, std::uint16_t type, std::string_view name) const;
    // The layout of a column, found once on an empty table.
    [[nodiscard]] ColumnLayout layoutOf(std::uint16_t type, std::string_view name) const;
    // The column of an empty table of the type: no row, and the layout of the column. The model of the caller is not
    // touched (reading a column of a card type that has no table makes the table).
    [[nodiscard]] QueryColumn emptyColumn(std::uint16_t type, std::string_view name) const;

private:
    struct Entry
    {
        std::string name;
        Fetcher fetcher;
    };
    [[nodiscard]] Fetcher fetcherOf(std::uint16_t type, std::string_view name) const;
    // The entry of a column by its name; the caller holds m_mutex.
    [[nodiscard]] const Entry* findEntry(std::uint16_t type, std::string_view name) const;

    mutable std::mutex m_mutex; // the layouts are filled in on demand
    mutable std::mutex m_scratchMutex;
    mutable std::unique_ptr<model::Model> m_scratch; // a model with no card: where the empty columns are read
    std::map<std::uint16_t, std::vector<Entry>> m_entries;
    mutable std::map<std::pair<std::uint16_t, std::string>, ColumnLayout> m_layouts;
};

} // namespace oor::query
