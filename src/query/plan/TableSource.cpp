#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/plan/QuerySource.hpp"

#include <unordered_map>

namespace oor::query
{

RowOrigin QuerySource::originOf(const RowSelection&) const
{
    return {};
}

std::unique_ptr<JoinTarget> QuerySource::joinTarget(std::string_view) const
{
    return nullptr;
}

std::vector<std::string> QuerySource::defaultJoinKeys(std::string_view) const
{
    return {};
}

std::string TableSource::description() const
{
    return "table of " + std::to_string(m_table->rowCount()) + " rows";
}

std::vector<ColumnDescription> TableSource::columns() const
{
    std::vector<ColumnDescription> result;
    for (std::size_t index = 0; index < m_table->columnCount(); ++index)
    {
        const ColumnInfo& info = m_table->info(index);
        result.push_back(ColumnDescription{
            info.name, info.origin == ColumnOrigin::Computed ? ColumnOrigin::Materialised : info.origin, info.field,
            true});
    }
    return result;
}

std::vector<NamedColumn> TableSource::fetch(std::string_view name, const RowSelection& rows,
                                            ExecutionContext& context) const
{
    const QueryColumn& column = m_table->column(name);
    std::vector<NamedColumn> result;
    if (rows.isAll())
        result.push_back(NamedColumn{std::string(name), column});
    else
        result.push_back(NamedColumn{std::string(name),
                                     take(column, std::span<const std::uint32_t>(rows.rows, rows.count), context)});
    return result;
}

RowOrigin TableSource::originOf(const RowSelection& rows) const
{
    const RowOrigin& origin = m_table->origin();
    if (!origin.isPresent() || rows.isAll())
        return origin;
    auto types = std::make_shared<std::vector<std::uint16_t>>(rows.count);
    auto keys = std::make_shared<std::vector<std::uint32_t>>(rows.count);
    for (std::size_t index = 0; index < rows.count; ++index)
    {
        (*types)[index] = (*origin.types)[rows.rows[index]];
        (*keys)[index] = (*origin.keys)[rows.rows[index]];
    }
    RowOrigin result;
    result.types = std::move(types);
    result.keys = std::move(keys);
    result.link = origin.link;
    return result;
}

namespace
{

// A table whose rows are found by an integer key.
class TableJoinTarget final : public JoinTarget
{
public:
    TableJoinTarget(std::shared_ptr<const QueryTable> table, const std::string& keyName) : m_table(std::move(table))
    {
        const QueryColumn& key = m_table->column(keyName);
        if (!isInteger(key.type()))
            throw QueryTypeError("the key '" + keyName + "' of the joined table must be integers, not " +
                                 std::string(columnTypeName(key.type())));
        const auto count = static_cast<std::size_t>(key.length());
        m_rows.reserve(count);
        for (std::size_t row = 0; row < count; ++row)
        {
            if (key.isNull(row))
                continue;
            std::int64_t value = 0;
            switch (key.type())
            {
            case ColumnType::Int32:
                value = key.as<std::int32_t>()[row];
                break;
            case ColumnType::UInt32:
                value = key.as<std::uint32_t>()[row];
                break;
            case ColumnType::UInt8:
                value = key.as<std::uint8_t>()[row];
                break;
            default:
                value = key.as<std::int64_t>()[row];
                break;
            }
            if (!m_rows.emplace(value, static_cast<std::uint32_t>(row)).second)
                throw QueryError("the key column '" + keyName +
                                 "' of the joined table is not unique: " + std::to_string(value) + " appears twice");
        }
    }

    [[nodiscard]] std::vector<ColumnDescription> columns() const override
    {
        std::vector<ColumnDescription> result;
        for (std::size_t index = 0; index < m_table->columnCount(); ++index)
            result.push_back(
                ColumnDescription{m_table->name(index), ColumnOrigin::Stored, m_table->info(index).field, true});
        return result;
    }

    [[nodiscard]] TargetRows locate(std::span<const std::int64_t> keys, ExecutionContext& context) const override
    {
        TargetRows matches;
        matches.rows.resize(keys.size());
        context.parallel(keys.size(), LIGHT_CHUNK,
                         [&](base::IndexRange range)
                         {
                             for (std::size_t index = range.begin; index < range.end; ++index)
                             {
                                 const auto found = m_rows.find(keys[index]);
                                 matches.rows[index] = found == m_rows.end() ? NO_ROW : found->second;
                             }
                         });
        return matches;
    }

    [[nodiscard]] std::vector<NamedColumn> fetch(std::string_view name, const TargetRows& matches,
                                                 ExecutionContext& context) const override
    {
        std::vector<NamedColumn> result;
        result.push_back(NamedColumn{std::string(name), take(m_table->column(name), matches.rows, context)});
        return result;
    }

private:
    std::shared_ptr<const QueryTable> m_table;
    std::unordered_map<std::int64_t, std::uint32_t> m_rows;
};

} // namespace

std::unique_ptr<JoinTarget> makeTableJoinTarget(std::shared_ptr<const QueryTable> table, const std::string& key)
{
    return std::make_unique<TableJoinTarget>(std::move(table), key);
}

} // namespace oor::query
