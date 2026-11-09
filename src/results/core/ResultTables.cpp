#include "oor/results/core/ResultTables.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <optional>
#include <set>

namespace oor::results
{

std::vector<std::int32_t> ResultContainer::subcases() const
{
    std::set<std::int32_t> found;
    for (const std::shared_ptr<ResultTable>& table : m_tables)
        found.insert(table->meta().subcase);
    return {found.begin(), found.end()};
}

std::vector<std::shared_ptr<ResultTable>> ResultContainer::tablesOf(std::int32_t subcase) const
{
    std::vector<std::shared_ptr<ResultTable>> found;
    for (const std::shared_ptr<ResultTable>& table : m_tables)
        if (table->meta().subcase == subcase)
            found.push_back(table);
    return found;
}

ResultContainer ResultContainer::ofAnalysis(std::int32_t analysisCode) const
{
    ResultContainer result(m_kind, m_elementName, m_variant);
    for (const std::shared_ptr<ResultTable>& table : m_tables)
        if (table->meta().analysisCode == analysisCode)
            result.add(table);
    return result;
}

std::vector<std::int32_t> ResultContainer::designCycles() const
{
    std::set<std::int32_t> found;
    for (const std::shared_ptr<ResultTable>& table : m_tables)
        if (table->meta().designCycle >= 0)
            found.insert(table->meta().designCycle);
    return {found.begin(), found.end()};
}

ResultContainer ResultContainer::ofDesignCycle(std::int32_t designCycle) const
{
    ResultContainer result(m_kind, m_elementName, m_variant);
    for (const std::shared_ptr<ResultTable>& table : m_tables)
        if (table->meta().designCycle == designCycle)
            result.add(table);
    return result;
}

ResultTables::ResultTables()
{
    // the containers of the kinds that are not families exist from the start, so that an empty one tells its kind
    for (const ResultKindInfo& info : allResultKinds())
        if (!info.family)
            m_containers.emplace(Key{0, static_cast<int>(info.kind), std::string()}, ResultContainer(info.kind));
}

const ResultContainer& ResultTables::container(ResultKind kind, ResultVariant variant) const noexcept
{
    static const ResultContainer NONE;
    if (resultKindInfo(kind).family)
        return NONE;
    const auto found = m_containers.find(Key{static_cast<int>(variant), static_cast<int>(kind), std::string()});
    if (found != m_containers.end())
        return found->second;
    // a variant that the file does not hold: an empty container that knows its kind
    static const std::map<std::pair<int, int>, ResultContainer> EMPTY = []
    {
        std::map<std::pair<int, int>, ResultContainer> empties;
        for (const ResultKindInfo& info : allResultKinds())
        {
            empties.emplace(std::make_pair(0, static_cast<int>(info.kind)), ResultContainer(info.kind));
            for (const ResultVariant each : allResultVariants())
                empties.emplace(std::make_pair(static_cast<int>(each), static_cast<int>(info.kind)),
                                ResultContainer(info.kind, each));
        }
        return empties;
    }();
    return EMPTY.at(std::make_pair(static_cast<int>(variant), static_cast<int>(kind)));
}

ResultContainer& ResultTables::mutableContainer(ResultKind kind, ResultVariant variant)
{
    if (resultKindInfo(kind).family)
        throw base::UsageError("a family kind has one container per element type or variant");
    const Key key{static_cast<int>(variant), static_cast<int>(kind), std::string()};
    const auto found = m_containers.find(key);
    if (found != m_containers.end())
        return found->second;
    return m_containers.emplace(key, ResultContainer(kind, variant)).first->second;
}

const ResultContainer& ResultTables::elementContainer(ResultKind kind, std::string_view elementName,
                                                      ResultVariant variant) const
{
    static const ResultContainer NONE;
    const auto found =
        m_containers.find(Key{static_cast<int>(variant), static_cast<int>(kind), std::string(elementName)});
    return found == m_containers.end() ? NONE : found->second;
}

ResultContainer& ResultTables::mutableElementContainer(ResultKind kind, const std::string& elementName,
                                                       ResultVariant variant)
{
    const Key key{static_cast<int>(variant), static_cast<int>(kind), elementName};
    const auto found = m_containers.find(key);
    if (found != m_containers.end())
        return found->second;
    return m_containers.emplace(key, ResultContainer(kind, elementName, variant)).first->second;
}

std::vector<std::string> ResultTables::elementNames(ResultKind kind, ResultVariant variant) const
{
    std::vector<std::string> names;
    for (const auto& [key, container] : m_containers)
        if (std::get<0>(key) == static_cast<int>(variant) && std::get<1>(key) == static_cast<int>(kind) &&
            !std::get<2>(key).empty() && container.tableCount() > 0)
            names.push_back(std::get<2>(key));
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::shared_ptr<ResultTable>> ResultTables::all() const
{
    std::vector<std::shared_ptr<ResultTable>> tables;
    for (const auto& entry : m_containers)
        tables.insert(tables.end(), entry.second.tables().begin(), entry.second.tables().end());
    return tables;
}

std::size_t ResultTables::tableCount() const noexcept
{
    std::size_t count = 0;
    for (const auto& entry : m_containers)
        count += entry.second.tableCount();
    return count;
}

std::vector<std::int32_t> ResultTables::designCycles() const
{
    std::set<std::int32_t> found;
    for (const auto& entry : m_containers)
        for (const std::int32_t cycle : entry.second.designCycles())
            found.insert(cycle);
    return {found.begin(), found.end()};
}

std::vector<std::shared_ptr<NamedResult>> ResultTables::named(std::string_view kind) const
{
    std::vector<std::shared_ptr<NamedResult>> found;
    for (const std::shared_ptr<NamedResult>& result : m_named)
        if (result->meta().kind == kind)
            found.push_back(result);
    return found;
}

ResultTables ResultTables::build(const std::shared_ptr<const ResultFile>& file, const ResultFilter& filter)
{
    return file ? file->tables(filter) : ResultTables();
}

namespace
{

// A table is wanted by the name of its kind ("stress"), by the name it has in its file, or, for an element table, by
// its element type ("ShellQuad4" as well as "stress.ShellQuad4"). A variant is named by its name ("psd") or with a kind
// ("psd.displacements", "psd.stress.ShellQuad4").
bool matchesTableFilter(const std::vector<std::string>& wanted, ResultKind kind, ResultVariant variant,
                        std::string_view containerName, std::string_view tableName)
{
    const std::string kindName(resultKindName(kind));
    const std::string_view variantName = resultVariantName(variant);
    for (const std::string& name : wanted)
    {
        if (base::equalsIgnoreCase(name, kindName) || (!tableName.empty() && base::equalsIgnoreCase(name, tableName)))
            return true;
        if (!variantName.empty())
        {
            const std::string prefix = std::string(variantName) + ".";
            if (base::equalsIgnoreCase(name, variantName) || base::equalsIgnoreCase(name, prefix + kindName))
                return true;
            if (!containerName.empty() &&
                base::equalsIgnoreCase(name, prefix + kindName + "." + std::string(containerName)))
                return true;
        }
        if (!containerName.empty() && (base::equalsIgnoreCase(name, containerName) ||
                                       base::equalsIgnoreCase(name, kindName + "." + std::string(containerName))))
            return true;
    }
    return false;
}

// True when a name of the `variants` filter names the variant ("" and "plain" are the results of an analysis).
bool variantWanted(const std::vector<std::string>& wanted, ResultVariant variant)
{
    return std::any_of(wanted.begin(), wanted.end(),
                       [&](const std::string& name)
                       {
                           if (variant == ResultVariant::Plain)
                               return name.empty() || base::equalsIgnoreCase(name, "plain");
                           return base::equalsIgnoreCase(name, resultVariantName(variant));
                       });
}

template <typename T>
bool keeps(const std::optional<std::vector<T>>& wanted, const T& value)
{
    return !wanted || std::find(wanted->begin(), wanted->end(), value) != wanted->end();
}

// The part of a filter that a table applies itself (steps, rows, components), without the members that choose tables.
ResultFilter selectionPart(const ResultFilter& filter)
{
    ResultFilter selection = filter;
    selection.subcases.reset();
    selection.tables.reset();
    selection.variants.reset();
    selection.designCycles.reset();
    return selection;
}

} // namespace

ResultTables ResultTables::fromTables(const std::vector<std::shared_ptr<ResultTable>>& tables,
                                      const std::vector<std::shared_ptr<NamedResult>>& named,
                                      const ResultFilter& filter)
{
    ResultTables result;
    const ResultFilter selection = selectionPart(filter);
    for (const std::shared_ptr<ResultTable>& table : tables)
    {
        const ResultTableMeta& meta = table->meta();
        const ResultKindInfo& info = resultKindInfo(meta.kind);
        const std::string& container = info.element ? meta.elementName : meta.subName;
        if (!keeps(filter.subcases, meta.subcase) || !keeps(filter.designCycles, meta.designCycle))
            continue;
        if (filter.tables && !matchesTableFilter(*filter.tables, meta.kind, meta.variant, container, meta.tableName))
            continue;
        if (filter.variants && !variantWanted(*filter.variants, meta.variant))
            continue;
        std::shared_ptr<ResultTable> kept = table;
        if (!selection.isEmpty())
        {
            try
            {
                kept = table->select(selection);
            }
            catch (const base::EmptySelectionError&)
            {
                continue; // the steps or components asked for are not in this table
            }
        }
        if (info.family)
            result.mutableElementContainer(meta.kind, container, meta.variant).add(std::move(kept));
        else
            result.mutableContainer(meta.kind, meta.variant).add(std::move(kept));
    }
    for (const std::shared_ptr<NamedResult>& item : named)
    {
        const NamedResultMeta& meta = item->meta();
        if (!keeps(filter.subcases, meta.subcase) || !keeps(filter.designCycles, meta.designCycle))
            continue;
        const auto namesIt = [&](const std::string& name)
        {
            return base::equalsIgnoreCase(name, meta.kind) || base::equalsIgnoreCase(name, meta.name) ||
                   base::equalsIgnoreCase(name, meta.tableName);
        };
        if (filter.tables && std::none_of(filter.tables->begin(), filter.tables->end(), namesIt))
            continue;
        result.addNamed(item);
    }
    const std::size_t candidates = tables.size() + named.size();
    if (candidates > 0 && result.tableCount() + result.namedCount() == 0 && !filter.isEmpty())
        throw base::EmptySelectionError("the filter (" + filter.describe() + ") keeps none of the " +
                                        std::to_string(candidates) + " tables of results the file holds");
    return result;
}

} // namespace oor::results
