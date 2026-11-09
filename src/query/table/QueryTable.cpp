#include "oor/query/table/QueryTable.hpp"

#include "oor/query/QueryErrors.hpp"

namespace oor::query
{

void QueryTable::addColumn(std::string name, QueryColumn column, ColumnOrigin origin, std::string field)
{
    if (find(name))
        throw QueryError("the table already has a column called '" + name + "'");
    if (m_columns.empty())
        m_rows = column.length();
    else if (column.length() != m_rows)
        throw QueryError("column '" + name + "' has " + std::to_string(column.length()) + " rows, the table has " +
                         std::to_string(m_rows));
    if (field.empty())
        field = name;
    m_infos.push_back(ColumnInfo{std::move(name), origin, std::move(field)});
    m_columns.push_back(std::move(column));
}

std::optional<std::size_t> QueryTable::find(std::string_view name) const noexcept
{
    for (std::size_t index = 0; index < m_infos.size(); ++index)
    {
        if (m_infos[index].name == name)
            return index;
    }
    return std::nullopt;
}

const QueryColumn& QueryTable::column(std::string_view name) const
{
    if (const auto index = find(name))
        return m_columns[*index];
    std::string known;
    for (const ColumnInfo& info : m_infos)
        known += (known.empty() ? "" : ", ") + info.name;
    throw QueryError("the table has no column '" + std::string(name) + "' (columns: " + known + ")");
}

std::vector<std::string> QueryTable::columnNames() const
{
    std::vector<std::string> names;
    names.reserve(m_infos.size());
    for (const ColumnInfo& info : m_infos)
        names.push_back(info.name);
    return names;
}

} // namespace oor::query
