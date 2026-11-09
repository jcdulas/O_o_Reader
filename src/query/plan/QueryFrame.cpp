#include "QueryFrame.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"

namespace oor::query
{

QueryFrame::QueryFrame(const QuerySource& source, ExecutionContext& context)
    : m_source(&source), m_context(&context), m_count(source.rowCount())
{
    for (ColumnDescription& description : source.columns())
    {
        Slot slot;
        slot.name = description.name;
        slot.sourceName = description.name;
        slot.origin = description.origin;
        slot.field = description.field.empty() ? description.name : description.field;
        slot.visible = description.byDefault;
        slot.kind = Kind::Source;
        m_slots.push_back(std::move(slot));
    }
    rebuildIndex();
}

QueryFrame::QueryFrame(ExecutionContext& context) : m_context(&context) {}

void QueryFrame::rebuildIndex()
{
    m_index.clear();
    for (std::size_t position = 0; position < m_slots.size(); ++position)
        m_index[m_slots[position].name] = position;
}

QueryFrame::Slot& QueryFrame::slotOf(std::string_view name)
{
    const auto found = m_index.find(std::string(name));
    if (found == m_index.end())
    {
        std::string known;
        std::size_t shown = 0;
        for (const Slot& slot : m_slots)
        {
            if (shown++ == 60)
            {
                known += ", ...";
                break;
            }
            known += (known.empty() ? "" : ", ") + slot.name;
        }
        throw QueryError("unknown column '" + std::string(name) + "' (columns: " + known + ")");
    }
    return m_slots[found->second];
}

const QueryFrame::Slot& QueryFrame::slotOf(std::string_view name) const
{
    return const_cast<QueryFrame*>(this)->slotOf(name);
}

bool QueryFrame::has(std::string_view name) const
{
    return m_index.find(std::string(name)) != m_index.end();
}

ColumnOrigin QueryFrame::originOf(std::string_view name) const
{
    return slotOf(name).origin;
}

ColumnInfo QueryFrame::infoOf(std::string_view name) const
{
    const Slot& slot = slotOf(name);
    return ColumnInfo{slot.name, slot.origin, slot.field};
}

bool QueryFrame::isExpensive(std::string_view name) const
{
    const Slot& slot = slotOf(name);
    if (slot.cache)
        return false;
    return slot.kind == Kind::Join || slot.origin == ColumnOrigin::Computed;
}

std::vector<std::string> QueryFrame::defaultColumns() const
{
    std::vector<std::string> names;
    for (const Slot& slot : m_slots)
    {
        if (slot.visible)
            names.push_back(slot.name);
    }
    return names;
}

std::vector<std::string> QueryFrame::allColumns() const
{
    std::vector<std::string> names;
    for (const Slot& slot : m_slots)
        names.push_back(slot.name);
    return names;
}

RowSelection QueryFrame::selection() const
{
    RowSelection rows;
    rows.count = m_count;
    rows.all = m_identity;
    rows.rows = m_identity ? nullptr : m_rows.data();
    return rows;
}

void QueryFrame::fill(Slot& slot)
{
    if (slot.cache)
        return;
    if (slot.kind == Kind::Value)
        throw QueryError("column '" + slot.name + "' has no values");
    m_context->checkCancelled();
    std::vector<NamedColumn> produced;
    if (slot.kind == Kind::Source)
    {
        if (slot.origin == ColumnOrigin::Computed)
            m_context->noteComputed(slot.name);
        m_context->beginStage(slot.origin == ColumnOrigin::Computed ? "computing " + slot.name : "reading " + slot.name,
                              m_count);
        produced = m_source->fetch(slot.sourceName, selection(), *m_context);
    }
    else
    {
        FrameJoin& join = m_joins[slot.join];
        if (slot.origin == ColumnOrigin::Computed)
            m_context->noteComputed(slot.name);
        m_context->beginStage("joining " + slot.name, m_count);
        produced = join.target->fetch(slot.sourceName, join.matches, *m_context);
    }
    bool found = false;
    for (NamedColumn& named : produced)
    {
        if (named.name == slot.sourceName)
        {
            slot.cache = std::move(named.column);
            found = true;
            continue;
        }
        // A column of the same computation: kept for whoever asks for it next.
        for (Slot& other : m_slots)
        {
            if (other.kind == slot.kind && other.join == slot.join && other.sourceName == named.name && !other.cache)
            {
                other.cache = std::move(named.column);
                break;
            }
        }
    }
    if (!found)
        throw QueryError("the source gave no values for column '" + slot.name + "'");
    if (slot.cache->length() != static_cast<std::int64_t>(m_count))
        throw QueryError("column '" + slot.name + "' has " + std::to_string(slot.cache->length()) + " values for " +
                         std::to_string(m_count) + " rows");
}

const QueryColumn& QueryFrame::column(std::string_view name)
{
    Slot& slot = slotOf(name);
    fill(slot);
    return *slot.cache;
}

ColumnLookup QueryFrame::lookup()
{
    return [this](const std::string& name) -> const QueryColumn* { return &column(name); };
}

void QueryFrame::narrow(std::span<const std::uint32_t> positions)
{
    if (m_source != nullptr)
    {
        if (m_identity)
            m_rows.assign(positions.begin(), positions.end());
        else
        {
            std::vector<std::uint32_t> rows(positions.size());
            for (std::size_t index = 0; index < positions.size(); ++index)
                rows[index] = m_rows[positions[index]];
            m_rows = std::move(rows);
        }
        m_identity = false;
    }
    for (Slot& slot : m_slots)
    {
        if (slot.cache)
            slot.cache = take(*slot.cache, positions, *m_context);
    }
    for (FrameJoin& join : m_joins)
    {
        TargetRows narrowed;
        narrowed.rows.resize(positions.size());
        if (!join.matches.types.empty())
            narrowed.types.resize(positions.size());
        for (std::size_t index = 0; index < positions.size(); ++index)
        {
            narrowed.rows[index] = join.matches.rows[positions[index]];
            if (!join.matches.types.empty())
                narrowed.types[index] = join.matches.types[positions[index]];
        }
        join.matches = std::move(narrowed);
    }
    m_count = positions.size();
}

void QueryFrame::addJoin(std::shared_ptr<JoinTarget> target, TargetRows matches,
                         const std::vector<JoinedColumn>& columns)
{
    const std::size_t join = m_joins.size();
    m_joins.push_back(FrameJoin{std::move(target), std::move(matches)});
    for (const JoinedColumn& column : columns)
    {
        Slot slot;
        slot.name = column.outputName;
        slot.sourceName = column.name;
        slot.origin = column.origin;
        slot.field = column.field;
        slot.visible = column.visible;
        slot.kind = Kind::Join;
        slot.join = join;
        m_slots.push_back(std::move(slot));
    }
    rebuildIndex();
}

void QueryFrame::addValue(std::string name, QueryColumn column, ColumnOrigin origin, std::string field, bool visible)
{
    if (has(name))
        throw QueryError("the query already has a column called '" + name + "'");
    Slot slot;
    slot.name = std::move(name);
    slot.origin = origin;
    slot.field = field.empty() ? slot.name : std::move(field);
    slot.visible = visible;
    slot.kind = Kind::Value;
    if (m_source == nullptr)
        m_count = column.size(); // a frame of columns in memory has the rows of its columns
    slot.cache = std::move(column);
    m_slots.push_back(std::move(slot));
    rebuildIndex();
}

void QueryFrame::project(const std::vector<Projection>& items)
{
    std::deque<Slot> kept;
    for (const Projection& item : items)
    {
        for (const Slot& other : kept)
        {
            if (other.name == item.name)
                throw QueryError("the select names column '" + item.name + "' twice");
        }
        if (item.value)
        {
            Slot slot;
            slot.name = item.name;
            slot.origin = ColumnOrigin::Derived;
            slot.field = item.name;
            slot.kind = Kind::Value;
            slot.cache = item.value;
            kept.push_back(std::move(slot));
            continue;
        }
        Slot slot = slotOf(item.from);
        slot.name = item.name;
        slot.visible = true;
        kept.push_back(std::move(slot));
    }
    m_slots = std::move(kept);
    rebuildIndex();
}

RowOrigin QueryFrame::rowOrigin() const
{
    if (m_source == nullptr)
        return {};
    return m_source->originOf(selection());
}

} // namespace oor::query
