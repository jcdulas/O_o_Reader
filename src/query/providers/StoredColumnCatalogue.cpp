#include "oor/query/providers/StoredColumnCatalogue.hpp"

#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/QueryErrors.hpp"

#include <mutex>

namespace oor::query
{

ColumnLayout ColumnLayout::of(const QueryColumn& column)
{
    ColumnLayout layout;
    layout.type = column.type();
    layout.width = column.width();
    if (column.child() != nullptr && column.type() != ColumnType::Dictionary)
    {
        layout.elementType = column.child()->type();
        layout.elementWidth = column.child()->width();
    }
    return layout;
}

namespace column_export
{
void registerCardColumns(StoredColumnCatalogue& catalogue); // src/query/providers/columns/RegisterCardColumns.cpp
}

StoredColumnCatalogue& StoredColumnCatalogue::instance()
{
    static StoredColumnCatalogue catalogue;
    // the stored columns of every card type, registered once, on first use
    static std::once_flag registered;
    std::call_once(registered, [] { column_export::registerCardColumns(catalogue); });
    return catalogue;
}

void StoredColumnCatalogue::add(std::uint16_t type, std::string name, Fetcher fetcher)
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<Entry>& entries = m_entries[type];
    for (Entry& entry : entries)
    {
        if (entry.name == name)
        {
            entry.fetcher = std::move(fetcher);
            return;
        }
    }
    entries.push_back(Entry{std::move(name), std::move(fetcher)});
}

std::vector<std::string> StoredColumnCatalogue::names(std::uint16_t type) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> result;
    if (const auto found = m_entries.find(type); found != m_entries.end())
    {
        for (const Entry& entry : found->second)
            result.push_back(entry.name);
    }
    return result;
}

const StoredColumnCatalogue::Entry* StoredColumnCatalogue::findEntry(std::uint16_t type, std::string_view name) const
{
    const auto found = m_entries.find(type);
    if (found == m_entries.end())
        return nullptr;
    for (const Entry& entry : found->second)
    {
        if (entry.name == name)
            return &entry;
    }
    return nullptr;
}

bool StoredColumnCatalogue::contains(std::uint16_t type, std::string_view name) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return findEntry(type, name) != nullptr;
}

StoredColumnCatalogue::Fetcher StoredColumnCatalogue::fetcherOf(std::uint16_t type, std::string_view name) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    const Entry* entry = findEntry(type, name);
    return entry != nullptr ? entry->fetcher : nullptr;
}

QueryColumn StoredColumnCatalogue::fetch(model::Model& model, std::uint16_t type, std::string_view name) const
{
    const Fetcher fetcher = fetcherOf(type, name);
    if (!fetcher)
        throw QueryError("the card type " + std::string(model::CardRegistry::instance().entry(type).info.name) +
                         " has no column '" + std::string(name) + "'");
    return fetcher(model);
}

QueryColumn StoredColumnCatalogue::emptyColumn(std::uint16_t type, std::string_view name) const
{
    const Fetcher fetcher = fetcherOf(type, name);
    if (!fetcher)
        throw QueryError("the card type " + std::string(model::CardRegistry::instance().entry(type).info.name) +
                         " has no column '" + std::string(name) + "'");
    const std::lock_guard<std::mutex> lock(m_scratchMutex);
    if (!m_scratch)
        m_scratch = std::make_unique<model::Model>();
    return fetcher(*m_scratch);
}

ColumnLayout StoredColumnCatalogue::layoutOf(std::uint16_t type, std::string_view name) const
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        const auto found = m_layouts.find({type, std::string(name)});
        if (found != m_layouts.end())
            return found->second;
    }
    // A table with no card has columns with the layout of the card type and no data to read.
    const ColumnLayout layout = ColumnLayout::of(emptyColumn(type, name));
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_layouts[{type, std::string(name)}] = layout;
    return layout;
}

} // namespace oor::query
