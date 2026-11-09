#pragma once

#include "oor/query/named/QueryInfo.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// Makes the table a named query returns, column by column from vectors. Every column of a table has the same length; a
// column that is not that long raises QueryError when it is added. `valid` (optional, one byte per row, 0 for a null)
// gives a column its nulls. A real column given with no `valid` has a null wherever its value is NaN (a quantity that
// does not apply to the row is null); the value stays NaN in the array so that NumPy shows it.
class TableBuilder
{
public:
    // `origin` and `field` say that the column is a field of the cards the rows come from (ColumnOrigin::Stored), or a
    // position in the basic system (Computed with field x, y, z), so that `apply` can write it.
    TableBuilder& int32(const std::string& name, std::vector<std::int32_t> values, std::vector<std::uint8_t> valid = {},
                        ColumnOrigin origin = ColumnOrigin::Derived, const std::string& field = {});
    TableBuilder& int64(const std::string& name, std::vector<std::int64_t> values,
                        std::vector<std::uint8_t> valid = {});
    TableBuilder& real(const std::string& name, std::vector<double> values, std::vector<std::uint8_t> valid = {},
                       ColumnOrigin origin = ColumnOrigin::Derived, const std::string& field = {});
    TableBuilder& flag(const std::string& name, const std::vector<std::uint8_t>& values,
                       std::vector<std::uint8_t> valid = {});
    TableBuilder& text(const std::string& name, const std::vector<std::string>& values,
                       std::vector<std::uint8_t> valid = {});
    // Texts with few distinct values (card type names): dictionary encoded, the dictionary in sorted order.
    TableBuilder& dictionary(const std::string& name, const std::vector<std::string>& values,
                             std::vector<std::uint8_t> valid = {});
    // The same from views of texts that outlive the table (the names of the card types, words of the cards): a row
    // costs a view, not a copy of its text.
    TableBuilder& dictionary(const std::string& name, const std::vector<std::string_view>& values,
                             std::vector<std::uint8_t> valid = {});
    // A list of IDs for each row, as compressed rows: row i has values[offsets[i] .. offsets[i + 1]).
    TableBuilder& intList(const std::string& name, std::vector<std::int64_t> offsets, std::vector<std::int32_t> values,
                          std::vector<std::uint8_t> valid = {});
    TableBuilder& intLists(const std::string& name, const std::vector<std::vector<std::int32_t>>& lists);
    TableBuilder& realList(const std::string& name, std::vector<std::int64_t> offsets, std::vector<double> values,
                           std::vector<std::uint8_t> valid = {});
    TableBuilder& textList(const std::string& name, std::vector<std::int64_t> offsets,
                           const std::vector<std::string>& values);
    // A column made elsewhere. `origin` Stored with `field` makes it a field of the cards the rows come from, which
    // `apply` can write (the table then needs `rows`).
    TableBuilder& column(const std::string& name, QueryColumn column, ColumnOrigin origin = ColumnOrigin::Derived,
                         const std::string& field = {});
    // The cards the rows come from (for the columns that are stored fields).
    TableBuilder& rows(std::vector<std::uint16_t> types, std::vector<std::uint32_t> keys,
                       std::shared_ptr<const ModelLink> link);

    [[nodiscard]] std::int64_t rowCount() const noexcept { return m_table.rowCount(); }
    [[nodiscard]] std::shared_ptr<const QueryTable> finish();

private:
    QueryTable m_table;
};

// A table with no row and the columns of a set, each with the type the catalogue declares for it.
[[nodiscard]] std::shared_ptr<const QueryTable> emptyTable(const ColumnSet& columns);

// A dictionary encoded column of texts (the card type names), the dictionary in sorted order.
// The same column from texts given by codes into `texts` (distinct; a negative code is a null): the dictionary of the
// texts the rows use, in the order of the texts.
[[nodiscard]] QueryColumn codedDictionaryColumn(const std::vector<std::string>& texts,
                                                const std::vector<std::int32_t>& codes);
[[nodiscard]] QueryColumn dictionaryColumn(const std::vector<std::string>& values,
                                           const std::vector<std::uint8_t>& valid = {});
[[nodiscard]] QueryColumn dictionaryColumn(const std::vector<std::string_view>& values,
                                           const std::vector<std::uint8_t>& valid = {});

} // namespace oor::query
