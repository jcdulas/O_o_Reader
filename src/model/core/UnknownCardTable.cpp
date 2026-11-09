#include "oor/model/core/UnknownCardTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

constexpr char SEPARATOR = '\x1f';

} // namespace

const CardTypeInfo& UnknownCardTable::info() const noexcept
{
    static const CardTypeInfo INFO{NAME, CardCategory::Unknown, base::IdSpace::None};
    return INFO;
}

std::uint32_t UnknownCardTable::append(base::CardKey key, const deck::CardName& name,
                                       std::span<const std::string_view> fields)
{
    const std::uint32_t row = m_rows.append(key);
    m_names.push_back(name);
    std::string joined;
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        if (index > 0)
            joined.push_back(SEPARATOR);
        joined.append(fields[index]);
    }
    m_text.push_back(std::move(joined));
    return row;
}

deck::CardName UnknownCardTable::nameAt(std::uint32_t row) const
{
    return m_names[row];
}

std::vector<std::string> UnknownCardTable::fieldsAt(std::uint32_t row) const
{
    std::vector<std::string> fields;
    const std::string& joined = m_text[row];
    std::size_t start = 0;
    while (true)
    {
        const std::size_t end = joined.find(SEPARATOR, start);
        fields.push_back(joined.substr(start, end == std::string::npos ? std::string::npos : end - start));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return fields;
}

void UnknownCardTable::compact()
{
    const std::vector<std::uint32_t> live = m_rows.liveRows();
    std::vector<deck::CardName> names;
    names.reserve(live.size());
    for (const std::uint32_t row : live)
        names.push_back(m_names[row]);
    m_names = std::move(names);
    std::vector<std::string> kept;
    kept.reserve(live.size());
    for (std::uint32_t row : live)
        kept.push_back(std::move(m_text[row]));
    m_text = std::move(kept);
    m_rows.applyCompaction(live);
}

} // namespace oor::model
