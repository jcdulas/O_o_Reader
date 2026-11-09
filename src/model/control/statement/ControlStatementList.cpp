#include "oor/model/control/statement/ControlStatementList.hpp"

#include <algorithm>
#include <utility>

namespace oor::model
{

void ControlStatementList::replaceWith(const ControlStatementList& other)
{
    m_entries = other.m_entries;
    m_nextKey = 1;
    for (ControlStatement& statement : m_entries)
        statement.key = newKey();
}

std::optional<std::size_t> ControlStatementList::indexOf(std::uint32_t key) const noexcept
{
    for (std::size_t index = 0; index < m_entries.size(); ++index)
    {
        if (m_entries[index].key == key)
            return index;
    }
    return std::nullopt;
}

ControlStatement* ControlStatementList::find(std::uint32_t key) noexcept
{
    const auto index = indexOf(key);
    return index ? &m_entries[*index] : nullptr;
}

const ControlStatement* ControlStatementList::find(std::uint32_t key) const noexcept
{
    const auto index = indexOf(key);
    return index ? &m_entries[*index] : nullptr;
}

const ControlStatement* ControlStatementList::findLast(std::string_view name) const noexcept
{
    for (auto statement = m_entries.rbegin(); statement != m_entries.rend(); ++statement)
    {
        if (statement->isTyped() && statement->name == name)
            return &*statement;
    }
    return nullptr;
}

ControlStatement* ControlStatementList::findLast(std::string_view name) noexcept
{
    return const_cast<ControlStatement*>(std::as_const(*this).findLast(name));
}

std::vector<const ControlStatement*> ControlStatementList::findAll(std::string_view name) const
{
    std::vector<const ControlStatement*> found;
    for (const ControlStatement& statement : m_entries)
    {
        if (statement.isTyped() && statement.name == name)
            found.push_back(&statement);
    }
    return found;
}

ControlStatement& ControlStatementList::insertAt(std::size_t index, ControlStatement statement)
{
    statement.key = newKey();
    const auto position = m_entries.begin() + static_cast<std::ptrdiff_t>(std::min(index, m_entries.size()));
    return *m_entries.insert(position, std::move(statement));
}

bool ControlStatementList::remove(std::uint32_t key)
{
    const auto index = indexOf(key);
    if (!index)
        return false;
    const ControlKind kind = m_entries[*index].kind;
    if (kind == ControlKind::IncludeBegin || kind == ControlKind::IncludeEnd)
        return false;
    m_entries.erase(m_entries.begin() + static_cast<std::ptrdiff_t>(*index));
    return true;
}

bool ControlStatementList::removeAll(std::string_view name)
{
    const std::size_t before = m_entries.size();
    std::erase_if(m_entries,
                  [name](const ControlStatement& statement) { return statement.isTyped() && statement.name == name; });
    return m_entries.size() != before;
}

std::size_t ControlStatementList::endIndex() const noexcept
{
    std::size_t end = m_entries.size();
    while (end > 0 && m_entries[end - 1].kind == ControlKind::Comment)
        --end;
    return end;
}

} // namespace oor::model
