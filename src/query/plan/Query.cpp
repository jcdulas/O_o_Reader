#include "oor/query/plan/Query.hpp"

#include "QueryFrame.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/operators/ExpressionEvaluator.hpp"
#include "oor/query/operators/GroupOperator.hpp"
#include "oor/query/operators/SortOperator.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <set>

namespace oor::query
{

Query Query::where(Expression condition) const
{
    return Query(m_source, m_plan.with(WhereStep{std::move(condition)}));
}

Query Query::join(JoinStep step) const
{
    return Query(m_source, m_plan.with(std::move(step)));
}

Query Query::select(std::vector<SelectItem> items) const
{
    if (items.empty())
        throw QueryError("select needs at least one column");
    return Query(m_source, m_plan.with(SelectStep{std::move(items)}));
}

Query Query::groupBy(std::vector<std::string> keys, std::vector<AggregateSpec> aggregates) const
{
    if (keys.empty() && aggregates.empty())
        throw QueryError("groupBy needs a key or an aggregate");
    return Query(m_source, m_plan.with(GroupStep{std::move(keys), std::move(aggregates)}));
}

Query Query::sortBy(std::vector<SortKey> keys) const
{
    if (keys.empty())
        throw QueryError("sort needs at least one column");
    return Query(m_source, m_plan.with(SortStep{std::move(keys)}));
}

Query Query::limit(std::size_t count) const
{
    return Query(m_source, m_plan.with(LimitStep{count}));
}

namespace
{

constexpr std::int64_t NO_KEY = std::numeric_limits<std::int64_t>::min();

// Throws QueryError, with the names of the columns, for a name the frame does not have.
void requireColumn(QueryFrame& frame, const std::string& name)
{
    if (!frame.has(name))
        (void)frame.column(name);
}

// --- where ----------------------------------------------------------------------------------------------------------

void runWhere(QueryFrame& frame, const WhereStep& step, ExecutionContext& context)
{
    if (step.condition.isEmpty())
        return;
    for (const std::string& name : step.condition.columns())
        requireColumn(frame, name);
    // Conditions on stored columns first: they are cheap and leave fewer rows for the ones that need a computation.
    std::vector<Expression> conditions = step.condition.conjuncts();
    std::stable_partition(conditions.begin(), conditions.end(),
                          [&](const Expression& condition)
                          {
                              for (const std::string& name : condition.columns())
                              {
                                  if (frame.isExpensive(name))
                                      return false;
                              }
                              return true;
                          });
    for (const Expression& condition : conditions)
    {
        context.checkCancelled();
        context.beginStage("filtering", frame.rowCount());
        const std::vector<std::uint32_t> rows =
            ExpressionEvaluator::selectRows(condition, frame.lookup(), frame.rowCount(), context);
        if (rows.size() != frame.rowCount())
            frame.narrow(rows);
    }
}

// --- join -----------------------------------------------------------------------------------------------------------

std::int64_t integerAt(const QueryColumn& column, std::size_t row) noexcept
{
    switch (column.type())
    {
    case ColumnType::Int32:
        return column.as<std::int32_t>()[row];
    case ColumnType::UInt32:
        return column.as<std::uint32_t>()[row];
    case ColumnType::UInt8:
        return column.as<std::uint8_t>()[row];
    default:
        return column.as<std::int64_t>()[row];
    }
}

std::string defaultPrefix(const std::string& target)
{
    return target == "node" || target == "nodes" ? "node" : "";
}

// The name of a joined column: the prefix, then the name with its first letter in capitals so that the result is
// camelCase ("node" and "x" give "nodeX"); a prefix that ends with '_' is put in front as it is.
std::string prefixedName(const std::string& prefix, const std::string& name)
{
    if (prefix.empty() || prefix.back() == '_' || name.empty())
        return prefix + name;
    return prefix + static_cast<char>(std::toupper(static_cast<unsigned char>(name.front()))) + name.substr(1);
}

void runJoin(QueryFrame& frame, const JoinStep& step, ExecutionContext& context)
{
    const QuerySource* source = frame.source();
    std::shared_ptr<JoinTarget> target;
    std::string targetName = step.target;
    if (step.table)
    {
        target = makeTableJoinTarget(step.table, step.tableKey);
        if (targetName.empty())
            targetName = "joined";
    }
    else
    {
        if (source == nullptr)
            throw QueryError("join '" + step.target +
                             "': the rows of a group are not cards, there is nothing to follow");
        target = source->joinTarget(step.target);
        if (!target)
            throw QueryError("join '" + step.target + "': " + source->description() +
                             " has no references to follow to it (property, material, node, element, coordinate)");
    }

    std::vector<std::string> on = step.on;
    if (on.empty())
    {
        if (step.table)
            throw QueryError("joining a table needs the column that holds its keys: give on=");
        for (const std::string& name : source->defaultJoinKeys(step.target))
        {
            if (frame.has(name))
                on.push_back(name);
        }
        if (on.empty())
            throw QueryError("join '" + step.target +
                             "': the query has none of the columns that hold the references (" +
                             "the source expects one of them); give on=");
    }
    std::vector<const QueryColumn*> keyColumns;
    for (const std::string& name : on)
        keyColumns.push_back(&frame.column(name));
    // A join to grid points, or on a column of lists, gives a row for each ID: every key column adds its IDs to the
    // rows. Any other join reads the key columns as one: the first with a value counts.
    const bool toNodes = step.target == "node" || step.target == "nodes" || step.target == "grid";
    bool hasList = false;
    for (std::size_t index = 0; index < keyColumns.size(); ++index)
    {
        const ColumnType type = keyColumns[index]->type();
        if (type == ColumnType::FixedList || type == ColumnType::List)
        {
            hasList = true;
            if (!isInteger(keyColumns[index]->child()->type()))
                throw QueryTypeError("join: the list column '" + on[index] + "' does not hold IDs");
        }
        else if (!isInteger(type))
            throw QueryTypeError("join: the column '" + on[index] + "' holds " + std::string(columnTypeName(type)) +
                                 ", not IDs");
    }
    const bool explode = hasList || toNodes;

    const std::string prefix = step.prefix.value_or(defaultPrefix(step.target));
    const bool isTable = step.table != nullptr;

    // The key of each row; with several IDs in a row, one row for each of them.
    std::vector<std::int64_t> keys;
    std::optional<std::string> positionName;
    if (explode)
    {
        std::vector<std::uint32_t> positions;
        std::vector<std::int32_t> indexes;
        const std::size_t rows = frame.rowCount();
        for (std::size_t row = 0; row < rows; ++row)
        {
            std::int64_t place = 0; // the position of the ID in the node list of the card, from 1
            for (const QueryColumn* column : keyColumns)
            {
                if (column->isNull(row))
                    continue;
                const bool isList = column->type() == ColumnType::FixedList || column->type() == ColumnType::List;
                const QueryColumn& values = isList ? *column->child() : *column;
                const std::int64_t first = isList ? column->listBegin(row) : static_cast<std::int64_t>(row);
                const std::int64_t count = isList ? column->listLength(row) : 1;
                for (std::int64_t at = 0; at < count; ++at)
                {
                    ++place;
                    const std::int64_t value = integerAt(values, static_cast<std::size_t>(first + at));
                    if (!isTable && value <= 0)
                        continue; // a blank place of the list (the absent mid-side node of a quadratic element)
                    positions.push_back(static_cast<std::uint32_t>(row));
                    keys.push_back(value);
                    indexes.push_back(static_cast<std::int32_t>(place));
                }
            }
        }
        frame.narrow(positions);
        positionName = prefixedName(prefix.empty() ? targetName : prefix, "position");
        if (frame.has(*positionName))
            throw QueryError("join: the query already has a column called '" + *positionName + "'");
        frame.addValue(*positionName, QueryColumn::primitive(std::move(indexes)), ColumnOrigin::Derived, {}, true);
    }
    else
    {
        const std::size_t rows = frame.rowCount();
        keys.assign(rows, NO_KEY);
        context.parallel(rows, LIGHT_CHUNK,
                         [&](base::IndexRange range)
                         {
                             for (const QueryColumn* column : keyColumns)
                             {
                                 for (std::size_t row = range.begin; row < range.end; ++row)
                                 {
                                     if (keys[row] != NO_KEY || column->isNull(row))
                                         continue;
                                     const std::int64_t value = integerAt(*column, row);
                                     if (isTable || value > 0)
                                         keys[row] = value;
                                 }
                             }
                         });
    }

    context.beginStage("joining " + targetName, keys.size());
    TargetRows matches = target->locate(keys, context);

    // The columns the join adds.
    const std::vector<ColumnDescription> available = target->columns();
    std::vector<QueryFrame::JoinedColumn> columns;
    const auto addColumn = [&](const ColumnDescription& description, bool visible)
    {
        QueryFrame::JoinedColumn column;
        column.name = description.name;
        column.outputName = prefixedName(prefix, description.name);
        if (frame.has(column.outputName))
        {
            if (step.prefix.has_value())
                throw QueryError("join: the query already has a column called '" + column.outputName + "'");
            column.outputName = prefixedName(targetName, description.name);
            if (frame.has(column.outputName))
                throw QueryError("join: the query already has a column called '" + column.outputName +
                                 "': give a prefix");
        }
        column.origin = description.origin == ColumnOrigin::Computed ? ColumnOrigin::Computed : ColumnOrigin::Joined;
        column.field = description.field.empty() ? description.name : description.field;
        column.visible = visible;
        columns.push_back(std::move(column));
    };
    if (step.columns.empty())
    {
        for (const ColumnDescription& description : available)
            addColumn(description, description.byDefault);
    }
    else
    {
        for (const std::string& name : step.columns)
        {
            const auto found =
                std::find_if(available.begin(), available.end(),
                             [&](const ColumnDescription& description) { return description.name == name; });
            if (found == available.end())
            {
                std::string known;
                for (const ColumnDescription& description : available)
                    known += (known.empty() ? "" : ", ") + description.name;
                throw QueryError("join '" + targetName + "' has no column '" + name + "' (columns: " + known + ")");
            }
            addColumn(*found, true);
        }
    }
    frame.addJoin(std::move(target), std::move(matches), columns);

    if (step.how == JoinKind::Inner)
    {
        const TargetRows& found = frame.lastMatches();
        std::vector<std::uint32_t> kept;
        kept.reserve(found.rows.size());
        for (std::size_t row = 0; row < found.rows.size(); ++row)
        {
            if (found.rows[row] != NO_ROW)
                kept.push_back(static_cast<std::uint32_t>(row));
        }
        if (kept.size() != found.rows.size())
            frame.narrow(kept);
    }
}

// --- select, group, sort, limit -------------------------------------------------------------------------------------

void runSelect(QueryFrame& frame, const SelectStep& step, ExecutionContext& context)
{
    std::vector<QueryFrame::Projection> projections;
    for (const SelectItem& item : step.items)
    {
        if (item.expression.isEmpty() && item.name == "*")
        {
            for (const std::string& name : frame.defaultColumns())
                projections.push_back(QueryFrame::Projection{name, name, std::nullopt});
            continue;
        }
        if (item.expression.isEmpty())
        {
            const std::string& from = item.from.empty() ? item.name : item.from;
            requireColumn(frame, from);
            projections.push_back(QueryFrame::Projection{item.name, from, std::nullopt});
            continue;
        }
        for (const std::string& name : item.expression.columns())
            requireColumn(frame, name);
        context.beginStage("computing " + item.name, frame.rowCount());
        QueryColumn value = ExpressionEvaluator::evaluate(item.expression, frame.lookup(), frame.rowCount(), context);
        projections.push_back(QueryFrame::Projection{item.name, std::string(), std::move(value)});
    }
    frame.project(projections);
}

std::unique_ptr<QueryFrame> runGroup(QueryFrame& frame, const GroupStep& step, ExecutionContext& context)
{
    std::set<std::string> outputs;
    for (const std::string& key : step.keys)
    {
        if (!outputs.insert(key).second)
            throw QueryError("groupBy: the key '" + key + "' is given twice");
    }
    for (const AggregateSpec& spec : step.aggregates)
    {
        if (!outputs.insert(spec.name).second)
            throw QueryError("groupBy: the name '" + spec.name + "' is used twice in the result");
    }
    std::vector<const QueryColumn*> keyColumns;
    for (const std::string& key : step.keys)
        keyColumns.push_back(&frame.column(key));
    context.beginStage("grouping", frame.rowCount());
    const GroupLayout layout = groupRows(keyColumns, frame.rowCount(), context);

    auto result = std::make_unique<QueryFrame>(context);
    std::vector<std::uint32_t> firstRows(layout.groupCount());
    if (!step.keys.empty())
    {
        for (std::size_t group = 0; group < firstRows.size(); ++group)
            firstRows[group] = layout.firstRow(group);
    }
    for (std::size_t index = 0; index < step.keys.size(); ++index)
    {
        result->addValue(step.keys[index], take(*keyColumns[index], firstRows, context), ColumnOrigin::Derived, {},
                         true);
    }
    for (const AggregateSpec& spec : step.aggregates)
    {
        if (spec.function == AggregateFunction::ArgMax || spec.function == AggregateFunction::ArgMin)
        {
            if (spec.column.empty() || spec.reported.empty())
                throw QueryError("groupBy: " + std::string(aggregateName(spec.function)) +
                                 " needs the column it compares and the column it reports, as argmax(vm, eid)");
            const std::vector<std::uint32_t> rows =
                extremeRows(spec.function == AggregateFunction::ArgMax, frame.column(spec.column), layout, context);
            result->addValue(spec.name, take(frame.column(spec.reported), rows, context), ColumnOrigin::Derived, {},
                             true);
            continue;
        }
        const QueryColumn* column = spec.column.empty() ? nullptr : &frame.column(spec.column);
        result->addValue(spec.name, aggregate(spec.function, column, layout, context), ColumnOrigin::Derived, {}, true);
    }
    return result;
}

void runSort(QueryFrame& frame, const SortStep& step, ExecutionContext& context)
{
    std::vector<SortColumn> keys;
    for (const SortKey& key : step.keys)
        keys.push_back(SortColumn{&frame.column(key.column), key.descending});
    const std::vector<std::uint32_t> order = sortedOrder(keys, frame.rowCount(), context);
    frame.narrow(order);
}

void runLimit(QueryFrame& frame, const LimitStep& step)
{
    if (step.count >= frame.rowCount())
        return;
    std::vector<std::uint32_t> first(step.count);
    for (std::size_t index = 0; index < first.size(); ++index)
        first[index] = static_cast<std::uint32_t>(index);
    frame.narrow(first);
}

} // namespace

std::shared_ptr<const QueryTable> Query::run(ExecutionContext& context) const
{
    auto frame = std::make_unique<QueryFrame>(*m_source, context);
    QueryReport report;
    report.sourceRows = static_cast<std::int64_t>(frame->rowCount());
    report.rowsAfterFilter = report.sourceRows;
    for (const QueryStep& step : m_plan.steps())
    {
        context.checkCancelled();
        if (const auto* where = std::get_if<WhereStep>(&step))
        {
            runWhere(*frame, *where, context);
            report.rowsAfterFilter = static_cast<std::int64_t>(frame->rowCount());
        }
        else if (const auto* join = std::get_if<JoinStep>(&step))
        {
            runJoin(*frame, *join, context);
            report.rowsAfterFilter = static_cast<std::int64_t>(frame->rowCount());
        }
        else if (const auto* select = std::get_if<SelectStep>(&step))
            runSelect(*frame, *select, context);
        else if (const auto* group = std::get_if<GroupStep>(&step))
            frame = runGroup(*frame, *group, context);
        else if (const auto* sort = std::get_if<SortStep>(&step))
            runSort(*frame, *sort, context);
        else if (const auto* limit = std::get_if<LimitStep>(&step))
            runLimit(*frame, *limit);
    }

    auto table = std::make_shared<QueryTable>();
    for (const std::string& name : frame->defaultColumns())
    {
        const ColumnInfo info = frame->infoOf(name);
        table->addColumn(name, frame->column(name), info.origin, info.field);
    }
    if (frame->hasSource())
        table->setOrigin(frame->rowOrigin());
    report.computedColumns = context.computed();
    table->setReport(std::move(report));
    table->setContractVersion(m_source->contractVersion());
    return table;
}

std::vector<std::string> Query::explain() const
{
    std::vector<std::string> lines;
    lines.push_back("source: " + m_source->description());
    for (const std::string& line : m_plan.describe())
        lines.push_back("  " + line);

    // The columns of the source the plan reads, and which of them are computed.
    std::vector<std::string> used;
    const auto note = [&](const std::string& name)
    {
        if (std::find(used.begin(), used.end(), name) == used.end())
            used.push_back(name);
    };
    bool selects = false;
    bool groups = false;
    for (const QueryStep& step : m_plan.steps())
    {
        if (const auto* where = std::get_if<WhereStep>(&step))
        {
            for (const std::string& name : where->condition.columns())
                note(name);
        }
        else if (const auto* join = std::get_if<JoinStep>(&step))
        {
            for (const std::string& name : join->on)
                note(name);
        }
        else if (const auto* select = std::get_if<SelectStep>(&step))
        {
            selects = true;
            for (const SelectItem& item : select->items)
            {
                if (item.expression.isEmpty())
                    note(item.from.empty() ? item.name : item.from);
                for (const std::string& name : item.expression.columns())
                    note(name);
            }
        }
        else if (const auto* group = std::get_if<GroupStep>(&step))
        {
            groups = true;
            for (const std::string& name : group->keys)
                note(name);
            for (const AggregateSpec& spec : group->aggregates)
            {
                if (!spec.column.empty())
                    note(spec.column);
                if (!spec.reported.empty())
                    note(spec.reported);
            }
        }
        else if (const auto* sort = std::get_if<SortStep>(&step))
        {
            for (const SortKey& key : sort->keys)
                note(key.column);
        }
    }
    std::string computed;
    std::string stored;
    for (const ColumnDescription& description : m_source->columns())
    {
        const bool isUsed = std::find(used.begin(), used.end(), description.name) != used.end();
        if (description.origin == ColumnOrigin::Computed)
        {
            if (isUsed)
                computed += (computed.empty() ? "" : ", ") + description.name;
        }
        else if (isUsed || (!selects && !groups))
            stored += (stored.empty() ? "" : ", ") + description.name;
    }
    lines.push_back("reads stored columns: " + (stored.empty() ? std::string("none") : stored));
    lines.push_back("evaluates computed columns: " + (computed.empty() ? std::string("none") : computed));
    return lines;
}

} // namespace oor::query
