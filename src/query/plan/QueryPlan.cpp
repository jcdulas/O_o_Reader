#include "oor/query/plan/QueryPlan.hpp"

namespace oor::query
{

std::string_view aggregateName(AggregateFunction function) noexcept
{
    switch (function)
    {
    case AggregateFunction::Count:
        return "count";
    case AggregateFunction::Sum:
        return "sum";
    case AggregateFunction::Min:
        return "min";
    case AggregateFunction::Max:
        return "max";
    case AggregateFunction::Mean:
        return "mean";
    case AggregateFunction::ArgMax:
        return "argmax";
    case AggregateFunction::ArgMin:
        return "argmin";
    }
    return "?";
}

std::optional<AggregateFunction> aggregateFromName(std::string_view name) noexcept
{
    for (const AggregateFunction function :
         {AggregateFunction::Count, AggregateFunction::Sum, AggregateFunction::Min, AggregateFunction::Max,
          AggregateFunction::Mean, AggregateFunction::ArgMax, AggregateFunction::ArgMin})
    {
        if (aggregateName(function) == name)
            return function;
    }
    return std::nullopt;
}

namespace
{

std::string joinNames(const std::vector<std::string>& names)
{
    std::string text;
    for (const std::string& name : names)
        text += (text.empty() ? "" : ", ") + name;
    return text;
}

struct Describer
{
    std::string operator()(const WhereStep& step) const { return "where " + step.condition.toString(); }
    std::string operator()(const JoinStep& step) const
    {
        std::string text = "join " + (step.table ? std::string("table") : step.target);
        if (!step.on.empty())
            text += " on " + joinNames(step.on);
        if (step.table)
            text += " = " + step.tableKey;
        text += step.how == JoinKind::Left ? " (left)" : " (inner)";
        return text;
    }
    std::string operator()(const SelectStep& step) const
    {
        std::string text = "select ";
        bool first = true;
        for (const SelectItem& item : step.items)
        {
            text += first ? "" : ", ";
            first = false;
            if (!item.expression.isEmpty())
                text += item.name + " = " + item.expression.toString();
            else if (!item.from.empty() && item.from != item.name)
                text += item.name + " = " + item.from;
            else
                text += item.name;
        }
        return text;
    }
    std::string operator()(const GroupStep& step) const
    {
        std::string text = "group by " + joinNames(step.keys) + ": ";
        bool first = true;
        for (const AggregateSpec& spec : step.aggregates)
        {
            text += first ? "" : ", ";
            first = false;
            text += spec.name + " = " + std::string(aggregateName(spec.function)) + "(" + spec.column +
                    (spec.reported.empty() ? "" : ", " + spec.reported) + ")";
        }
        return text;
    }
    std::string operator()(const SortStep& step) const
    {
        std::string text = "sort by ";
        bool first = true;
        for (const SortKey& key : step.keys)
        {
            text += first ? "" : ", ";
            first = false;
            text += key.column + (key.descending ? " descending" : "");
        }
        return text;
    }
    std::string operator()(const LimitStep& step) const { return "limit " + std::to_string(step.count); }
};

} // namespace

std::vector<std::string> QueryPlan::describe() const
{
    std::vector<std::string> lines;
    for (const QueryStep& step : m_steps)
        lines.push_back(std::visit(Describer{}, step));
    return lines;
}

} // namespace oor::query
