#include "oor/query/named/common/CardList.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/providers/CardColumns.hpp"

#include <algorithm>
#include <cctype>
#include <numeric>

namespace oor::query
{

CardList listCards(const model::Model& model, std::span<const std::uint16_t> types,
                   const std::optional<std::vector<std::int64_t>>& only)
{
    struct Entry
    {
        std::uint16_t type;
        std::uint32_t row;
        std::uint32_t key;
        std::int32_t id;
    };
    const std::optional<std::vector<std::int64_t>> wanted = sortedIds(only);
    std::vector<Entry> entries;
    std::size_t alive = 0;
    for (const std::uint16_t type : types)
    {
        if (const model::CardTable* table = model.findTable(type))
            alive += table->rows().aliveCount();
    }
    entries.reserve(wanted ? std::min(alive, wanted->size()) : alive);
    for (const std::uint16_t type : types)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        const model::RowBook& rows = table->rows();
        const std::size_t before = entries.size();
        for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
        {
            if (!rows.isAlive(row))
                continue;
            const std::int32_t id = table->idAt(row);
            if (wanted && !std::binary_search(wanted->begin(), wanted->end(), static_cast<std::int64_t>(id)))
                continue;
            entries.push_back(Entry{type, row, rows.keyAt(row).value, id});
        }
        // Each table is in key order: merging keeps file order for cards that share an ID.
        if (before > 0)
            std::inplace_merge(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(before), entries.end(),
                               [](const Entry& left, const Entry& right) { return left.key < right.key; });
    }
    const auto byId = [](const Entry& left, const Entry& right) { return left.id < right.id; };
    if (!std::is_sorted(entries.begin(), entries.end(), byId)) // a mesh numbered in order needs no sort
        std::stable_sort(entries.begin(), entries.end(), byId);
    CardList list;
    list.types.reserve(entries.size());
    list.rows.reserve(entries.size());
    list.ids.reserve(entries.size());
    list.keys.reserve(entries.size());
    for (const Entry& entry : entries)
    {
        list.types.push_back(entry.type);
        list.rows.push_back(entry.row);
        list.ids.push_back(entry.id);
        list.keys.push_back(entry.key);
    }
    return list;
}

std::vector<std::uint16_t> typesInSpace(model::CardCategory category, base::IdSpace space)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<std::uint16_t> types;
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTypeInfo& info = registry.entry(type).info;
        if (info.category == category && info.idSpace == space)
            types.push_back(type);
    }
    return types;
}

std::vector<std::uint16_t> typesNamed(const std::vector<std::string>& names, model::CardCategory category,
                                      const std::string& what)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<std::uint16_t> types;
    for (const std::string& name : names)
    {
        // A card type by its neutral name ("IsoMat").
        const std::optional<std::uint16_t> type = registry.typeIndexOf(name);
        if (!type || registry.entry(*type).info.category != category)
            throw QueryError(what + ": '" + name + "' is not a card type of the right kind");
        types.push_back(*type);
    }
    return types;
}

std::string_view typeName(std::uint16_t type)
{
    return model::CardRegistry::instance().entry(type).info.name;
}

std::optional<std::vector<std::int64_t>> sortedIds(const std::optional<std::vector<std::int64_t>>& ids)
{
    if (!ids)
        return std::nullopt;
    std::vector<std::int64_t> sorted = *ids;
    std::sort(sorted.begin(), sorted.end());
    sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
    return sorted;
}

ListOrigin originOf(const CardList& list)
{
    return ListOrigin{list.types, list.keys};
}

ListColumns::ListColumns(std::shared_ptr<model::Model> model, const CardList& list, ExecutionContext& context)
    : m_model(std::move(model)), m_list(list), m_context(context)
{
    m_distinctTypes = list.types;
    std::sort(m_distinctTypes.begin(), m_distinctTypes.end());
    m_distinctTypes.erase(std::unique(m_distinctTypes.begin(), m_distinctTypes.end()), m_distinctTypes.end());
}

bool ListColumns::has(const std::string& name)
{
    if (m_distinctTypes.empty())
        return false;
    if (!m_engine)
        m_engine = std::make_unique<CardColumns>(m_model, m_distinctTypes);
    return m_engine->has(name);
}

const QueryColumn& ListColumns::get(const std::string& name)
{
    const auto cached = m_columns.find(name);
    if (cached != m_columns.end())
        return cached->second;
    if (m_distinctTypes.empty())
    {
        // No card: an empty column of the type the engine would give.
        return m_columns.emplace(name, QueryColumn::primitive(std::vector<double>())).first->second;
    }
    if (!m_engine)
        m_engine = std::make_unique<CardColumns>(m_model, m_distinctTypes);
    if (name == "propId" && !m_engine->has("propId"))
    {
        // None of the card types has a property (point masses, springs, rigid elements): every value is null.
        const std::vector<std::uint8_t> none(m_list.size(), 0);
        std::int64_t nullCount = 0;
        ColumnBuffer bits = validityFromFlags(none, nullCount);
        QueryColumn column = QueryColumn::primitive(std::vector<std::int32_t>(m_list.size(), 0));
        return m_columns.emplace(name, nullCount == 0 ? column : column.withValidity(std::move(bits), nullCount))
            .first->second;
    }
    CardRows rows;
    if (m_distinctTypes.size() == 1)
        rows.single = m_distinctTypes.front();
    else
        rows.types = m_list.types;
    rows.rows = m_list.rows;
    std::vector<NamedColumn> made = m_engine->fetch(name, rows, m_context);
    for (NamedColumn& column : made)
        m_columns.emplace(column.name, std::move(column.column));
    const auto found = m_columns.find(name);
    if (found == m_columns.end())
        throw QueryError("the column '" + name + "' is not available for these cards");
    return found->second;
}

std::vector<double> ListColumns::reals(const std::string& name)
{
    const QueryColumn& column = get(name);
    std::vector<double> values(column.size());
    copyAsDouble(column, 0, column.size(), values.data());
    return values;
}

std::vector<std::int32_t> ListColumns::integers(const std::string& name)
{
    const QueryColumn& column = get(name);
    std::vector<std::int32_t> values(column.size(), 0);
    if (column.type() != ColumnType::Int32)
        return values;
    const auto data = column.as<std::int32_t>();
    for (std::size_t row = 0; row < values.size(); ++row)
        values[row] = column.isNull(row) ? 0 : data[row];
    return values;
}

NodeLists nodesOfCards(const model::Model& model, const CardList& list, ExecutionContext& context)
{
    const std::size_t count = list.size();
    const std::size_t chunk = 16384;
    const std::size_t chunks = (count + chunk - 1) / chunk;
    std::vector<std::vector<std::int32_t>> parts(chunks);
    std::vector<std::vector<std::int64_t>> sizes(chunks);
    context.parallel(count, chunk,
                     [&](base::IndexRange range)
                     {
                         const std::size_t index = range.begin / chunk;
                         std::vector<base::EntityId> nodes;
                         for (std::size_t row = range.begin; row < range.end; ++row)
                         {
                             const std::size_t before = parts[index].size();
                             const model::CardTable* table = model.findTable(list.types[row]);
                             if (table != nullptr && analysis::ElementTopology::supports(list.types[row]))
                             {
                                 nodes.clear();
                                 analysis::ElementTopology::nodesOf(*table, list.types[row], list.rows[row], nodes);
                                 parts[index].insert(parts[index].end(), nodes.begin(), nodes.end());
                             }
                             sizes[index].push_back(static_cast<std::int64_t>(parts[index].size() - before));
                         }
                     });
    NodeLists result;
    result.offsets.reserve(count + 1);
    result.offsets.push_back(0);
    for (std::size_t index = 0; index < chunks; ++index)
    {
        for (const std::int64_t size : sizes[index])
            result.offsets.push_back(result.offsets.back() + size);
        result.nodes.insert(result.nodes.end(), parts[index].begin(), parts[index].end());
    }
    return result;
}

} // namespace oor::query
