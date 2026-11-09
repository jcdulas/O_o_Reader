#include "oor/query/named/QueryInfo.hpp"

namespace oor::query
{

std::string_view columnKindName(ColumnKind kind) noexcept
{
    switch (kind)
    {
    case ColumnKind::Int32:
        return "int32";
    case ColumnKind::Int64:
        return "int64";
    case ColumnKind::Float64:
        return "float64";
    case ColumnKind::Bool:
        return "bool";
    case ColumnKind::Text:
        return "utf8";
    case ColumnKind::Dictionary:
        return "dictionary<utf8>";
    case ColumnKind::IntList:
        return "list<int32>";
    case ColumnKind::RealList:
        return "list<float64>";
    case ColumnKind::TextList:
        return "list<utf8>";
    case ColumnKind::Result:
        return "float32 or float64";
    case ColumnKind::Step:
        return "steps";
    }
    return "?";
}

const ColumnSet& QueryInfo::columnsFor(const std::string& detail) const
{
    for (const ColumnSet& set : columnSets)
    {
        if (set.detail == detail)
            return set;
    }
    return columnSets.front();
}

ColumnListBuilder& ColumnListBuilder::int32(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Int32, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::int64(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Int64, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::real(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Float64, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::flag(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Bool, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::text(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Text, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::dictionary(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Dictionary, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::intList(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::IntList, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::realList(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::RealList, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::textList(const std::string& name, bool editable)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::TextList, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::result(const std::string& name)
{
    m_columns.push_back(ColumnSpec{name, ColumnKind::Result, false});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::results(const std::string& names)
{
    std::size_t start = 0;
    while (start < names.size())
    {
        const std::size_t end = names.find(' ', start);
        const std::size_t stop = end == std::string::npos ? names.size() : end;
        if (stop > start)
            m_columns.push_back(ColumnSpec{names.substr(start, stop - start), ColumnKind::Result, false});
        start = stop + 1;
    }
    return *this;
}

ColumnListBuilder& ColumnListBuilder::step()
{
    m_columns.push_back(ColumnSpec{"[step]", ColumnKind::Step, false});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::vector3(const std::string& prefix, const std::string& suffixes, bool editable)
{
    for (const char suffix : suffixes)
        m_columns.push_back(ColumnSpec{prefix + std::string(1, suffix), ColumnKind::Float64, editable});
    return *this;
}

ColumnListBuilder& ColumnListBuilder::reals(const std::string& names, bool editable)
{
    std::size_t start = 0;
    while (start < names.size())
    {
        const std::size_t end = names.find(' ', start);
        const std::size_t stop = end == std::string::npos ? names.size() : end;
        if (stop > start)
            m_columns.push_back(ColumnSpec{names.substr(start, stop - start), ColumnKind::Float64, editable});
        start = stop + 1;
    }
    return *this;
}

ColumnSet ColumnListBuilder::build(std::string detail) const
{
    return ColumnSet{std::move(detail), m_columns};
}

ParameterInfo parameter(std::string name, ParameterKind kind, std::string defaultText, std::string description)
{
    return ParameterInfo{std::move(name), kind, std::move(defaultText), std::move(description)};
}

} // namespace oor::query
