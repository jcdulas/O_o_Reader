#include "oor/query/operators/EnvelopeOperator.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/operators/SortOperator.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>

namespace oor::query
{

namespace
{

std::string joinNames(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

bool isStepColumn(const std::string& name)
{
    return name == "time" || name == "freq" || name == "mode" || name == "loadStep" || name == "eigenvalue";
}

bool isListType(ColumnType type)
{
    return type == ColumnType::List || type == ColumnType::FixedList;
}

const QueryColumn& requireColumn(const QueryTable& table, std::string_view name, const char* what)
{
    if (!table.has(name))
        throw QueryError(std::string("envelope: ") + what + " '" + std::string(name) +
                         "' is not a column of the table (its columns: " + joinNames(table.columnNames()) + ")");
    return table.column(name);
}

} // namespace

std::string_view envelopeByName(EnvelopeBy by) noexcept
{
    switch (by)
    {
    case EnvelopeBy::Max:
        return "max";
    case EnvelopeBy::Min:
        return "min";
    case EnvelopeBy::AbsMax:
        return "absMax";
    }
    return "max";
}

EnvelopeBy envelopeByFromName(std::string_view name)
{
    if (name == "max")
        return EnvelopeBy::Max;
    if (name == "min")
        return EnvelopeBy::Min;
    if (name == "absMax" || name == "absmax")
        return EnvelopeBy::AbsMax;
    throw QueryError("envelope: by is \"max\", \"min\" or \"absMax\", not '" + std::string(name) + "'");
}

std::shared_ptr<const QueryTable> envelopeTable(const QueryTable& table, const EnvelopeSpec& spec,
                                                ExecutionContext& context)
{
    const QueryColumn& valueColumn = requireColumn(table, spec.value, "the value column");
    if (!isNumeric(valueColumn.type()))
        throw QueryError("envelope: the value column '" + spec.value + "' is not a number (" +
                         std::string(columnTypeName(valueColumn.type())) + ")");
    if (spec.over.empty())
        throw QueryError(
            "envelope: over names at least one column (the analysis, subcase or step the envelope is "
            "taken over)");
    std::set<std::string> overSet;
    for (const std::string& name : spec.over)
    {
        (void)requireColumn(table, name, "the column the envelope is over");
        overSet.insert(name);
    }
    if (overSet.contains(spec.value))
        throw QueryError("envelope: the value column '" + spec.value + "' cannot be one the envelope is over");

    // the rows of the analyses asked for
    std::vector<std::uint32_t> kept(static_cast<std::size_t>(table.rowCount()));
    std::iota(kept.begin(), kept.end(), 0U);
    if (!spec.analyses.empty())
    {
        const QueryColumn& labels = requireColumn(table, "analysis", "the analysis column");
        std::set<std::string> present;
        for (std::size_t row = 0; row < labels.size(); ++row)
            present.insert(std::string(labels.textAt(row)));
        std::vector<std::string> wanted;
        for (const std::string& label : spec.analyses)
        {
            if (!present.contains(label))
                throw QueryError("envelope: the table has no analysis labelled '" + label + "' (its analyses: " +
                                 joinNames(std::vector<std::string>(present.begin(), present.end())) + ")");
            wanted.push_back(label);
        }
        std::vector<std::uint32_t> narrowed;
        for (const std::uint32_t row : kept)
            if (std::find(wanted.begin(), wanted.end(), std::string(labels.textAt(row))) != wanted.end())
                narrowed.push_back(row);
        kept = std::move(narrowed);
    }

    // the keys
    std::vector<std::string> keyNames = spec.keys;
    if (keyNames.empty())
    {
        for (std::size_t index = 0; index < table.columnCount(); ++index)
        {
            const std::string& name = table.name(index);
            if (overSet.contains(name) || name == spec.value || name == "source" || name == "frame" ||
                name == "envelopeCount")
                continue;
            const ColumnType type = table.column(index).type();
            if (isListType(type))
                continue;
            if (isFloating(type) && !isStepColumn(name))
                continue;
            keyNames.push_back(name);
        }
    }
    else
    {
        for (const std::string& name : keyNames)
        {
            const QueryColumn& column = requireColumn(table, name, "the key column");
            if (isListType(column.type()))
                throw QueryError("envelope: the key column '" + name + "' is a list");
            if (name == spec.value)
                throw QueryError("envelope: the value column '" + spec.value + "' cannot also be a key");
            if (overSet.contains(name))
                throw QueryError("envelope: '" + name + "' is both a key and a column the envelope is over");
        }
    }

    // the rows to work on, as columns of their own
    const auto count = kept.size();
    QueryTable work;
    for (std::size_t index = 0; index < table.columnCount(); ++index)
        work.addColumn(table.name(index), take(table.column(index), kept, context));
    std::vector<SortColumn> sorts;
    for (const std::string& name : keyNames)
        sorts.push_back(SortColumn{&work.column(name), false});
    std::vector<std::uint32_t> order(count);
    if (sorts.empty())
        std::iota(order.begin(), order.end(), 0U);
    else
        order = sortedOrder(sorts, count, context);
    std::vector<double> values(count);
    copyAsDouble(work.column(spec.value), 0, count, values.data());

    // the critical row of each group
    const auto better = [&](double candidate, double current)
    {
        if (std::isnan(candidate))
            return false;
        if (std::isnan(current))
            return true;
        switch (spec.by)
        {
        case EnvelopeBy::Max:
            return candidate > current;
        case EnvelopeBy::Min:
            return candidate < current;
        case EnvelopeBy::AbsMax:
            return std::fabs(candidate) > std::fabs(current);
        }
        return false;
    };
    std::vector<std::uint32_t> picked;
    std::vector<std::int64_t> groupSize;
    std::size_t at = 0;
    while (at < count)
    {
        std::size_t end = at + 1;
        while (end < count)
        {
            bool same = true;
            for (const SortColumn& key : sorts)
                if (compareKeys(*key.column, order[at], order[end]) != 0)
                {
                    same = false;
                    break;
                }
            if (!same)
                break;
            ++end;
        }
        std::uint32_t best = order[at];
        for (std::size_t position = at + 1; position < end; ++position)
            if (better(values[order[position]], values[best]))
                best = order[position];
        picked.push_back(best);
        groupSize.push_back(static_cast<std::int64_t>(end - at));
        at = end;
    }

    auto result = std::make_shared<QueryTable>();
    for (std::size_t index = 0; index < work.columnCount(); ++index)
        result->addColumn(work.name(index), take(work.column(index), picked, context));
    result->addColumn("envelopeCount", QueryColumn::primitive(std::move(groupSize)));
    result->setSources(table.sources());
    result->setContractVersion(table.contractVersion());
    return result;
}

} // namespace oor::query
