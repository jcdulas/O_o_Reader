#include "oor/query/providers/CardColumns.hpp"

#include "ComputedColumns.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <optional>

namespace oor::query
{

namespace
{

bool isListLike(ColumnType type) noexcept
{
    return type == ColumnType::FixedList || type == ColumnType::List;
}

// The layout that the columns of one name have once the types are put together: the layout of the first card type, or a
// list when the types have lists of the same values with different widths.
ColumnLayout mergeLayouts(const std::vector<ColumnLayout>& layouts)
{
    const ColumnLayout& first = layouts.front();
    ColumnLayout merged = first;
    if (isListLike(first.type))
    {
        for (const ColumnLayout& layout : layouts)
        {
            if (isListLike(layout.type) && layout.elementType == first.elementType &&
                layout.elementWidth == first.elementWidth && !(layout == first))
            {
                merged.type = ColumnType::List;
                merged.width = 0;
            }
        }
    }
    return merged;
}

// True when the column of a card type has a layout that goes with the merged one: the same, or a list of the same
// values.
bool contributes(const ColumnLayout& layout, const ColumnLayout& merged)
{
    if (layout == merged)
        return true;
    return merged.type == ColumnType::List && isListLike(layout.type) && layout.elementType == merged.elementType &&
           layout.elementWidth == merged.elementWidth;
}

} // namespace

CardColumns::CardColumns(std::shared_ptr<model::Model> model, std::vector<std::uint16_t> types)
    : m_model(std::move(model)), m_types(std::move(types))
{
    std::sort(m_types.begin(), m_types.end());
    m_types.erase(std::unique(m_types.begin(), m_types.end()), m_types.end());
    const std::vector<std::uint16_t> requested = m_types;
    // Of several card types (a category), only those that have a table can have cards; a single card type is kept even
    // without one, so that its columns are known (the table is made when a column is read).
    if (m_types.size() > 1)
        m_types.erase(std::remove_if(m_types.begin(), m_types.end(),
                                     [&](std::uint16_t type) { return m_model->findTable(type) == nullptr; }),
                      m_types.end());
    std::size_t offset = 0;
    for (const std::uint16_t type : m_types)
    {
        const model::CardTable* table = m_model->findTable(type);
        const std::size_t rows = table != nullptr ? table->rows().rowCount() : 0;
        m_tableRows.push_back(rows);
        m_offsets.push_back(offset);
        offset += rows;
    }
    // A category of which the model has no card still has its columns (the union of those of all its card types), so
    // that a query written for one model runs on any model and gives a table with no row.
    m_layoutTypes = m_types.empty() ? requested : m_types;
    const StoredColumnCatalogue& catalogue = StoredColumnCatalogue::instance();
    std::vector<std::string> order;
    for (const std::uint16_t type : m_layoutTypes)
    {
        for (const std::string& name : catalogue.names(type))
        {
            if (std::find(order.begin(), order.end(), name) == order.end())
                order.push_back(name);
        }
    }
    for (const std::string& name : order)
    {
        std::vector<ColumnLayout> layouts;
        std::vector<std::uint16_t> owners;
        for (const std::uint16_t type : m_layoutTypes)
        {
            if (catalogue.contains(type, name))
            {
                layouts.push_back(catalogue.layoutOf(type, name));
                owners.push_back(type);
            }
        }
        // When the card types disagree about a column (a number in one, a list in another), the layout of the first one
        // wins and the cards of the others have a null.
        Stored stored;
        stored.name = name;
        stored.layout = mergeLayouts(layouts);
        for (std::size_t index = 0; index < layouts.size(); ++index)
        {
            if (contributes(layouts[index], stored.layout))
                stored.contributing.push_back(owners[index]);
        }
        m_stored.push_back(std::move(stored));
    }
    if (std::any_of(m_stored.begin(), m_stored.end(), [](const Stored& stored) { return stored.name == "type"; }))
        m_typeColumnName = "card";
}

std::vector<ColumnDescription> CardColumns::columns() const
{
    std::vector<ColumnDescription> result;
    for (const Stored& stored : m_stored)
        result.push_back(ColumnDescription{stored.name, ColumnOrigin::Stored, stored.name, true});
    result.push_back(ColumnDescription{m_typeColumnName, ColumnOrigin::Derived, m_typeColumnName, true});
    const std::vector<std::string> computed =
        detail::computedNamesFor(m_layoutTypes, [](std::uint16_t type, std::string_view name)
                                 { return StoredColumnCatalogue::instance().contains(type, name); });
    for (const std::string& name : computed)
    {
        // A card that has a field of this name keeps it: the stored column wins.
        if (std::any_of(m_stored.begin(), m_stored.end(), [&](const Stored& stored) { return stored.name == name; }))
            continue;
        if (name == m_typeColumnName)
            continue;
        result.push_back(ColumnDescription{name, ColumnOrigin::Computed, name, false});
    }
    return result;
}

bool CardColumns::has(std::string_view name) const
{
    const std::vector<ColumnDescription> all = columns();
    return std::any_of(all.begin(), all.end(),
                       [&](const ColumnDescription& description) { return description.name == name; });
}

QueryColumn CardColumns::typeColumn(const CardRows& rows) const
{
    std::vector<std::string> names;
    for (const std::uint16_t type : m_types)
        names.emplace_back(model::CardRegistry::instance().entry(type).info.name);
    const std::size_t count = rows.count();
    std::vector<std::int32_t> indices(count, 0);
    bool anyMissing = false;
    std::vector<std::uint8_t> present(count, 1);
    // A card kept as text is named as the deck names it (`CQUAD9`, never `UNKNOWN`): its name
    // joins the dictionary after the names of the card types.
    const auto* unknown = !m_types.empty() && m_types.front() == model::CardRegistry::UNKNOWN_TYPE
                              ? static_cast<const model::UnknownCardTable*>(m_model->findTable(m_types.front()))
                              : nullptr;
    const auto unknownNameIndex = [&](std::uint32_t row)
    {
        const std::string name(unknown->nameAt(row).view());
        const auto found = std::find(names.begin(), names.end(), name);
        if (found != names.end())
            return static_cast<std::int32_t>(found - names.begin());
        names.push_back(name);
        return static_cast<std::int32_t>(names.size() - 1);
    };
    if (!rows.wholeTable || unknown != nullptr)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            if (rows.rows[index] == NO_ROW)
            {
                present[index] = 0;
                anyMissing = true;
                continue;
            }
            if (unknown != nullptr && rows.typeAt(index) == model::CardRegistry::UNKNOWN_TYPE)
            {
                indices[index] = unknownNameIndex(rows.rows[index]);
                continue;
            }
            const auto position = std::lower_bound(m_types.begin(), m_types.end(), rows.typeAt(index));
            indices[index] = static_cast<std::int32_t>(position - m_types.begin());
        }
    }
    QueryColumn column = QueryColumn::dictionary(std::move(indices), names);
    if (anyMissing)
    {
        std::int64_t nullCount = 0;
        ColumnBuffer bits = validityFromFlags(present, nullCount);
        column = column.withValidity(std::move(bits), nullCount);
    }
    return column;
}

QueryColumn CardColumns::fetchStored(std::string_view name, const CardRows& rows, ExecutionContext& context) const
{
    const StoredColumnCatalogue& catalogue = StoredColumnCatalogue::instance();
    const auto stored =
        std::find_if(m_stored.begin(), m_stored.end(), [&](const Stored& entry) { return entry.name == name; });
    if (m_types.empty() || (m_types.size() == 1 && m_model->findTable(m_types[0]) == nullptr))
    {
        // No card of these types: a column of no row. The model is not touched (reading the column of a card type that
        // has no table would make the table).
        for (const std::uint16_t type : m_layoutTypes)
        {
            if (!catalogue.contains(type, name))
                continue;
            QueryColumn empty = catalogue.emptyColumn(type, name);
            if (stored->layout.type == ColumnType::List && empty.type() == ColumnType::FixedList)
                empty = fixedListAsList(empty);
            return take(empty, rows.rows, context);
        }
    }
    // The column of one type for some of its rows, as a list when the types put together have lists of several widths
    // (the rows taken first: the lists of the rows asked are made, not those of the table).
    const auto takeOf = [&](std::uint16_t type, std::span<const std::uint32_t> wanted)
    {
        QueryColumn part = take(catalogue.fetch(*m_model, type, name), wanted, context);
        if (stored->layout.type == ColumnType::List && part.type() == ColumnType::FixedList)
            part = fixedListAsList(part);
        return part;
    };
    const auto contributes = [&](std::uint16_t type)
    {
        return catalogue.contains(type, name) &&
               std::find(stored->contributing.begin(), stored->contributing.end(), type) != stored->contributing.end();
    };
    if (m_types.size() == 1)
    {
        if (!rows.wholeTable)
            return takeOf(m_types[0], rows.rows);
        QueryColumn full = catalogue.fetch(*m_model, m_types[0], name);
        if (stored->layout.type == ColumnType::List && full.type() == ColumnType::FixedList)
            full = fixedListAsList(full);
        return full;
    }
    // Several types, fewer rows than the tables hold (`table(ids=...)` on a large model): the rows of each type taken
    // from its column, put end to end, then in the order asked. The columns of every row of the tables are not put
    // together for some of them.
    const std::size_t tableRows = m_types.empty() ? 0 : m_offsets.back() + m_tableRows.back();
    if (rows.count() < tableRows)
    {
        std::vector<std::vector<std::uint32_t>> local(m_types.size());
        std::vector<std::uint32_t> partOf(rows.count(), NO_ROW); // the type of each row asked, as a position in m_types
        std::vector<std::uint32_t> global(rows.count(), NO_ROW); // its rank among the rows of its type, then its row
        for (std::size_t index = 0; index < rows.count(); ++index)
        {
            if (rows.rows[index] == NO_ROW)
                continue;
            const auto position = static_cast<std::size_t>(
                std::lower_bound(m_types.begin(), m_types.end(), rows.typeAt(index)) - m_types.begin());
            partOf[index] = static_cast<std::uint32_t>(position);
            global[index] = static_cast<std::uint32_t>(local[position].size());
            local[position].push_back(rows.rows[index]);
        }
        std::vector<QueryColumn> parts(m_types.size());
        std::vector<const QueryColumn*> pointers(m_types.size(), nullptr);
        std::vector<std::int64_t> lengths(m_types.size(), 0);
        std::vector<std::uint32_t> starts(m_types.size(), 0);
        std::uint32_t start = 0;
        std::optional<std::size_t> firstContributing;
        for (std::size_t position = 0; position < m_types.size(); ++position)
        {
            starts[position] = start;
            lengths[position] = static_cast<std::int64_t>(local[position].size());
            start += static_cast<std::uint32_t>(local[position].size());
            if (!contributes(m_types[position]))
                continue;
            if (!firstContributing)
                firstContributing = position;
            if (local[position].empty())
                continue;
            parts[position] = takeOf(m_types[position], local[position]);
            pointers[position] = &parts[position];
        }
        if (firstContributing && pointers[*firstContributing] == nullptr &&
            std::all_of(pointers.begin(), pointers.end(), [](const QueryColumn* part) { return part == nullptr; }))
        {
            // no row asked of a type that has the column: an empty part gives the layout of the nulls
            parts[*firstContributing] = takeOf(m_types[*firstContributing], {});
            pointers[*firstContributing] = &parts[*firstContributing];
        }
        if (firstContributing)
        {
            const QueryColumn joined = concatenate(pointers, lengths);
            for (std::size_t index = 0; index < global.size(); ++index)
            {
                if (partOf[index] != NO_ROW)
                    global[index] += starts[partOf[index]];
            }
            return take(joined, global, context);
        }
    }
    // Several types: one column of all the rows of all the tables, put together once and kept for this source.
    QueryColumn joined;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        const auto found = m_joined.find(std::string(name));
        if (found != m_joined.end())
            joined = found->second;
        else
        {
            std::vector<QueryColumn> parts;
            parts.reserve(m_types.size());
            std::vector<bool> present;
            for (const std::uint16_t type : m_types)
            {
                if (!contributes(type))
                {
                    parts.emplace_back();
                    present.push_back(false);
                    continue;
                }
                QueryColumn part = catalogue.fetch(*m_model, type, name);
                if (stored->layout.type == ColumnType::List && part.type() == ColumnType::FixedList)
                    part = fixedListAsList(part);
                parts.push_back(std::move(part));
                present.push_back(true);
            }
            std::vector<const QueryColumn*> pointers;
            std::vector<std::int64_t> lengths;
            for (std::size_t index = 0; index < parts.size(); ++index)
            {
                pointers.push_back(present[index] ? &parts[index] : nullptr);
                lengths.push_back(static_cast<std::int64_t>(m_tableRows[index]));
            }
            joined = concatenate(pointers, lengths);
            m_joined.emplace(std::string(name), joined);
        }
    }
    std::vector<std::uint32_t> global(rows.count());
    for (std::size_t index = 0; index < global.size(); ++index)
    {
        if (rows.rows[index] == NO_ROW)
        {
            global[index] = NO_ROW;
            continue;
        }
        const auto position = std::lower_bound(m_types.begin(), m_types.end(), rows.typeAt(index));
        global[index] = static_cast<std::uint32_t>(m_offsets[static_cast<std::size_t>(position - m_types.begin())] +
                                                   rows.rows[index]);
    }
    return take(joined, global, context);
}

analysis::MassOptions CardColumns::massOptions(ExecutionContext&) const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    analysis::MassOptions options;
    if (!m_massResolved)
    {
        // The set of non-structural mass that the Case Control selects, as `element.mass()` does by default.
        if (const auto set = analysis::NonStructuralMass::selectedByCaseControl(*m_model))
            m_nonStructural = analysis::NonStructuralMass::resolve(*m_model, *set);
        m_massResolved = true;
    }
    if (m_nonStructural)
        options.nonStructural = &*m_nonStructural;
    return options;
}

std::vector<NamedColumn> CardColumns::fetch(std::string_view name, const CardRows& rows,
                                            ExecutionContext& context) const
{
    std::vector<NamedColumn> result;
    if (std::any_of(m_stored.begin(), m_stored.end(), [&](const Stored& stored) { return stored.name == name; }))
    {
        result.push_back(NamedColumn{std::string(name), fetchStored(name, rows, context)});
        return result;
    }
    if (name == m_typeColumnName)
    {
        result.push_back(NamedColumn{std::string(name), typeColumn(rows)});
        return result;
    }
    const std::optional<detail::Family> family = detail::familyOf(name);
    if (!family || !has(name))
        throw QueryError("unknown column '" + std::string(name) + "'");
    const analysis::MassOptions options =
        *family == detail::Family::Mass ? massOptions(context) : analysis::MassOptions{};
    return detail::computeColumns(name, *m_model, rows, context, options);
}

} // namespace oor::query
