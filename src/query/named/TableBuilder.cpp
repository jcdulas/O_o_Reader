#include "oor/query/named/TableBuilder.hpp"

#include "oor/model/DataContract.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <map>

namespace oor::query
{

namespace
{

QueryColumn withNulls(QueryColumn column, const std::vector<std::uint8_t>& valid)
{
    if (valid.empty())
        return column;
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(valid, nullCount);
    if (nullCount == 0)
        return column;
    return column.withValidity(std::move(bits), nullCount);
}

} // namespace

QueryColumn dictionaryColumn(const std::vector<std::string_view>& values, const std::vector<std::uint8_t>& valid)
{
    // The views of one text are mostly the same view in runs: a row whose view is that of the row before takes its
    // entry without a lookup.
    std::map<std::string_view, std::int32_t> position;
    std::string_view previous;
    bool started = false;
    for (std::size_t row = 0; row < values.size(); ++row)
    {
        if (!valid.empty() && valid[row] == 0)
            continue;
        if (!started || previous.data() != values[row].data() || previous.size() != values[row].size())
            position.emplace(values[row], 0);
        previous = values[row];
        started = true;
    }
    std::vector<std::string> dictionary;
    dictionary.reserve(position.size());
    for (auto& entry : position)
    {
        entry.second = static_cast<std::int32_t>(dictionary.size());
        dictionary.emplace_back(entry.first);
    }
    std::vector<std::int32_t> indices(values.size(), 0);
    started = false;
    std::int32_t previousIndex = 0;
    for (std::size_t row = 0; row < values.size(); ++row)
    {
        if (!valid.empty() && valid[row] == 0)
            continue;
        if (!started || previous.data() != values[row].data() || previous.size() != values[row].size())
            previousIndex = position.at(values[row]);
        indices[row] = previousIndex;
        previous = values[row];
        started = true;
    }
    return withNulls(QueryColumn::dictionary(std::move(indices), dictionary), valid);
}

QueryColumn codedDictionaryColumn(const std::vector<std::string>& texts, const std::vector<std::int32_t>& codes)
{
    std::vector<std::uint8_t> used(texts.size(), 0);
    std::vector<std::uint8_t> valid(codes.size(), 1);
    bool nulls = false;
    for (std::size_t row = 0; row < codes.size(); ++row)
    {
        if (codes[row] < 0)
        {
            valid[row] = 0;
            nulls = true;
        }
        else
            used[static_cast<std::size_t>(codes[row])] = 1;
    }
    std::map<std::string, std::int32_t> position;
    for (std::size_t code = 0; code < texts.size(); ++code)
        if (used[code] != 0)
            position.emplace(texts[code], 0);
    std::vector<std::string> dictionary;
    dictionary.reserve(position.size());
    for (auto& entry : position)
    {
        entry.second = static_cast<std::int32_t>(dictionary.size());
        dictionary.push_back(entry.first);
    }
    std::vector<std::int32_t> entryOf(texts.size(), 0);
    for (std::size_t code = 0; code < texts.size(); ++code)
        if (used[code] != 0)
            entryOf[code] = position.at(texts[code]);
    std::vector<std::int32_t> indices(codes.size(), 0);
    for (std::size_t row = 0; row < codes.size(); ++row)
        if (codes[row] >= 0)
            indices[row] = entryOf[static_cast<std::size_t>(codes[row])];
    return withNulls(QueryColumn::dictionary(std::move(indices), dictionary),
                     nulls ? valid : std::vector<std::uint8_t>{});
}

QueryColumn dictionaryColumn(const std::vector<std::string>& values, const std::vector<std::uint8_t>& valid)
{
    // A text column repeats its values in runs (the type, the frame, the source of the rows of one table): a row equal
    // to the one before takes its entry without a lookup.
    std::map<std::string, std::int32_t> position;
    const std::string* previous = nullptr;
    for (std::size_t row = 0; row < values.size(); ++row)
    {
        if (!valid.empty() && valid[row] == 0)
            continue;
        if (previous == nullptr || *previous != values[row])
            position.emplace(values[row], 0);
        previous = &values[row];
    }
    std::vector<std::string> dictionary;
    dictionary.reserve(position.size());
    for (auto& entry : position)
    {
        entry.second = static_cast<std::int32_t>(dictionary.size());
        dictionary.push_back(entry.first);
    }
    std::vector<std::int32_t> indices(values.size(), 0);
    previous = nullptr;
    std::int32_t previousIndex = 0;
    for (std::size_t row = 0; row < values.size(); ++row)
    {
        if (!valid.empty() && valid[row] == 0)
            continue;
        if (previous == nullptr || *previous != values[row])
            previousIndex = position.at(values[row]);
        indices[row] = previousIndex;
        previous = &values[row];
    }
    return withNulls(QueryColumn::dictionary(std::move(indices), dictionary), valid);
}

std::shared_ptr<const QueryTable> emptyTable(const ColumnSet& columns)
{
    TableBuilder builder;
    for (const ColumnSpec& spec : columns.columns)
    {
        switch (spec.kind)
        {
        case ColumnKind::Int32:
            builder.int32(spec.name, {});
            break;
        case ColumnKind::Int64:
            builder.int64(spec.name, {});
            break;
        case ColumnKind::Float64:
            builder.real(spec.name, {});
            break;
        case ColumnKind::Bool:
            builder.flag(spec.name, {});
            break;
        case ColumnKind::Text:
            builder.text(spec.name, {});
            break;
        case ColumnKind::Dictionary:
            builder.dictionary(spec.name, std::vector<std::string>{});
            break;
        case ColumnKind::IntList:
            builder.intList(spec.name, {0}, {});
            break;
        case ColumnKind::RealList:
            builder.realList(spec.name, {0}, {});
            break;
        case ColumnKind::TextList:
            builder.textList(spec.name, {0}, {});
            break;
        case ColumnKind::Result:
            builder.real(spec.name, {});
            break;
        case ColumnKind::Step:
            break; // the columns of the steps exist only when the data has steps
        }
    }
    return builder.finish();
}

TableBuilder& TableBuilder::int32(const std::string& name, std::vector<std::int32_t> values,
                                  std::vector<std::uint8_t> valid, ColumnOrigin origin, const std::string& field)
{
    m_table.addColumn(name, withNulls(QueryColumn::primitive(std::move(values)), valid), origin, field);
    return *this;
}

TableBuilder& TableBuilder::int64(const std::string& name, std::vector<std::int64_t> values,
                                  std::vector<std::uint8_t> valid)
{
    m_table.addColumn(name, withNulls(QueryColumn::primitive(std::move(values)), valid));
    return *this;
}

TableBuilder& TableBuilder::real(const std::string& name, std::vector<double> values, std::vector<std::uint8_t> valid,
                                 ColumnOrigin origin, const std::string& field)
{
    if (valid.empty())
    {
        bool anyNaN = false;
        for (const double value : values)
            anyNaN = anyNaN || value != value;
        if (anyNaN)
        {
            valid.resize(values.size());
            for (std::size_t row = 0; row < values.size(); ++row)
                valid[row] = values[row] == values[row] ? 1 : 0;
        }
    }
    m_table.addColumn(name, withNulls(QueryColumn::primitive(std::move(values)), valid), origin, field);
    return *this;
}

TableBuilder& TableBuilder::flag(const std::string& name, const std::vector<std::uint8_t>& values,
                                 std::vector<std::uint8_t> valid)
{
    m_table.addColumn(name, withNulls(QueryColumn::boolean(values), valid));
    return *this;
}

TableBuilder& TableBuilder::text(const std::string& name, const std::vector<std::string>& values,
                                 std::vector<std::uint8_t> valid)
{
    m_table.addColumn(name, withNulls(QueryColumn::utf8(values), valid));
    return *this;
}

TableBuilder& TableBuilder::dictionary(const std::string& name, const std::vector<std::string>& values,
                                       std::vector<std::uint8_t> valid)
{
    m_table.addColumn(name, dictionaryColumn(values, valid));
    return *this;
}

TableBuilder& TableBuilder::dictionary(const std::string& name, const std::vector<std::string_view>& values,
                                       std::vector<std::uint8_t> valid)
{
    m_table.addColumn(name, dictionaryColumn(values, valid));
    return *this;
}

TableBuilder& TableBuilder::intList(const std::string& name, std::vector<std::int64_t> offsets,
                                    std::vector<std::int32_t> values, std::vector<std::uint8_t> valid)
{
    const auto length = static_cast<std::int64_t>(offsets.size()) - 1;
    QueryColumn column =
        QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), QueryColumn::primitive(std::move(values)), length);
    m_table.addColumn(name, withNulls(std::move(column), valid));
    return *this;
}

TableBuilder& TableBuilder::intLists(const std::string& name, const std::vector<std::vector<std::int32_t>>& lists)
{
    std::vector<std::int64_t> offsets;
    offsets.reserve(lists.size() + 1);
    offsets.push_back(0);
    std::vector<std::int32_t> values;
    for (const auto& list : lists)
    {
        values.insert(values.end(), list.begin(), list.end());
        offsets.push_back(static_cast<std::int64_t>(values.size()));
    }
    return intList(name, std::move(offsets), std::move(values));
}

TableBuilder& TableBuilder::realList(const std::string& name, std::vector<std::int64_t> offsets,
                                     std::vector<double> values, std::vector<std::uint8_t> valid)
{
    const auto length = static_cast<std::int64_t>(offsets.size()) - 1;
    QueryColumn column =
        QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), QueryColumn::primitive(std::move(values)), length);
    m_table.addColumn(name, withNulls(std::move(column), valid));
    return *this;
}

TableBuilder& TableBuilder::textList(const std::string& name, std::vector<std::int64_t> offsets,
                                     const std::vector<std::string>& values)
{
    const auto length = static_cast<std::int64_t>(offsets.size()) - 1;
    QueryColumn column = QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), QueryColumn::utf8(values), length);
    m_table.addColumn(name, std::move(column));
    return *this;
}

TableBuilder& TableBuilder::column(const std::string& name, QueryColumn column, ColumnOrigin origin,
                                   const std::string& field)
{
    m_table.addColumn(name, std::move(column), origin, field);
    return *this;
}

TableBuilder& TableBuilder::rows(std::vector<std::uint16_t> types, std::vector<std::uint32_t> keys,
                                 std::shared_ptr<const ModelLink> link)
{
    RowOrigin origin;
    origin.types = std::make_shared<const std::vector<std::uint16_t>>(std::move(types));
    origin.keys = std::make_shared<const std::vector<std::uint32_t>>(std::move(keys));
    origin.link = std::move(link);
    m_table.setOrigin(std::move(origin));
    return *this;
}

std::shared_ptr<const QueryTable> TableBuilder::finish()
{
    m_table.setContractVersion(model::DATA_CONTRACT_VERSION);
    return std::make_shared<const QueryTable>(std::move(m_table));
}

} // namespace oor::query
