#include "OptimisationData.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

std::set<std::int64_t> idSet(const ResultQueryContext& context, const char* name)
{
    std::set<std::int64_t> ids;
    if (const auto list = context.arguments().ids(name))
        ids.insert(list->begin(), list->end());
    return ids;
}

// The block of the rows of one result set: its origin, precision and status come from the first row.
template <typename Row>
void describeBlock(RowBlock& block, const ResultQueryContext& context, std::size_t set, const Row& first)
{
    block.origin(set, context.sets()[set].analysis, -1);
    block.precision(first.precision);
    block.validation(first.status);
}

// ---- designHistory
// -----------------------------------------------------------------------------------------------------

class DesignHistoryQuery final : public ResultQuery
{
public:
    DesignHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        if (data.cycles().empty())
            return emptyTable(info().defaultColumns());
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<const DesignCycle*> cycles;
            for (const DesignCycle& item : data.cycles())
                if (item.set == set)
                    cycles.push_back(&item);
            if (cycles.empty())
                continue;
            const std::size_t count = cycles.size();
            std::vector<std::int32_t> number(count), violatedCount(count, 0);
            std::vector<std::uint8_t> countValid(count, 0), converged(count, 0), convergedValid(count, 0);
            std::vector<double> objective(count), violation(count, NOT_A_NUMBER);
            std::vector<std::string> analysis(count, context.sets()[set].analysis);
            std::vector<std::int16_t> provider(count);
            for (std::size_t row = 0; row < count; ++row)
            {
                const DesignCycle& item = *cycles[row];
                number[row] = item.cycle;
                objective[row] = item.objective;
                provider[row] = sources.of(item.source);
                bool any = false;
                bool counted = false;
                double worst = 0.0;
                std::int32_t violated = 0;
                for (const DesignConstraint& constraint : data.constraints())
                {
                    if (constraint.set != set || constraint.cycle != item.cycle || std::isnan(constraint.violation))
                        continue;
                    any = true;
                    worst = std::max(worst, constraint.violation);
                    if (constraint.source == "constraints on the response values")
                    {
                        counted = true;
                        if (constraint.violation > 0.0)
                            ++violated;
                    }
                }
                if (any)
                    violation[row] = worst;
                if (counted)
                {
                    violatedCount[row] = violated;
                    countValid[row] = 1;
                }
                if (item.convergenceKnown)
                {
                    converged[row] = item.converged ? 1 : 0;
                    convergedValid[row] = 1;
                }
            }
            RowBlock block(count);
            block.texts("analysis", std::move(analysis));
            block.ints("designCycle", std::move(number));
            block.reals("objective", std::move(objective));
            block.reals("maxConstraintViolation", std::move(violation));
            block.ints("violatedConstraintCount", std::move(violatedCount), std::move(countValid));
            block.flags("isConverged", std::move(converged), std::move(convergedValid));
            block.providers(std::move(provider));
            describeBlock(block, context, set, *cycles.front());
            accumulator.add(std::move(block));
        }
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designHistory";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "One row for each design cycle (iteration) of an optimisation: the objective, the largest violation of a "
            "constraint "
            "(normalised, zero when none is violated; null when the tables give no constraint), the number of violated "
            "constraints (null when only the most critical constraint of the cycle is known) and whether the cycle "
            "ended in a soft or hard convergence. It reads the history of the iterations (`designHistory`) or the "
            "design variable history (`designVariableHistory`), or the responses a solver gives in memory.";
        info.defaultSort = "analysis, designCycle";
        ColumnListBuilder columns;
        columns.text("analysis").int32("designCycle").result("objective").real("maxConstraintViolation");
        columns.int32("violatedConstraintCount").flag("isConverged");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- designVariableHistory
// --------------------------------------------------------------------------------------------

class DesignVariableHistoryQuery final : public ResultQuery
{
public:
    DesignVariableHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const std::set<std::int64_t> wanted = idSet(context, "ids");
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<std::int32_t> cycle, id;
            std::vector<std::string> label;
            std::vector<double> value, lower, upper, change;
            std::vector<std::int16_t> provider;
            const DesignCycle* first = nullptr;
            // the variables are listed cycle after cycle in the order of their table
            std::map<std::int32_t, double> previous;
            for (const DesignCycle& item : data.cycles())
            {
                if (item.set != set)
                    continue;
                first = first != nullptr ? first : &item;
                std::vector<std::int32_t> order;
                for (const DesignVariable& variable : data.variables(set))
                    if (item.values.contains(variable.id))
                        order.push_back(variable.id);
                for (const auto& entry : item.values)
                    if (std::find(order.begin(), order.end(), entry.first) == order.end())
                        order.push_back(entry.first);
                for (const std::int32_t variableId : order)
                {
                    if (!wanted.empty() && !wanted.contains(variableId))
                        continue;
                    const DesignVariable* variable = data.variable(set, variableId);
                    cycle.push_back(item.cycle);
                    id.push_back(variableId);
                    label.push_back(variable != nullptr ? variable->label : std::string());
                    const double current = item.values.at(variableId);
                    value.push_back(current);
                    lower.push_back(variable != nullptr ? variable->lower : NOT_A_NUMBER);
                    upper.push_back(variable != nullptr ? variable->upper : NOT_A_NUMBER);
                    const auto before = previous.find(variableId);
                    change.push_back(before == previous.end() ? NOT_A_NUMBER : current - before->second);
                    provider.push_back(sources.of(item.source));
                }
                previous = item.values;
            }
            if (first == nullptr || cycle.empty())
                continue;
            RowBlock block(cycle.size());
            block.ints("designCycle", std::move(cycle));
            block.ints("id", std::move(id));
            block.texts("label", std::move(label));
            block.reals("value", std::move(value));
            block.reals("lower", std::move(lower));
            block.reals("upper", std::move(upper));
            block.reals("change", std::move(change));
            block.providers(std::move(provider));
            describeBlock(block, context, set, *first);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designVariableHistory";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "The value of each design variable at each design cycle, with its label and bounds (from the design "
            "variables, `designVariables`, null where the results have none) and its change from the previous cycle "
            "(null at the first). `ids` "
            "keeps "
            "some variables.";
        info.parameters = {parameter("ids", ParameterKind::IdList, "None", "the design variables to list")};
        info.defaultSort = "designCycle, id";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("id")
            .dictionary("label")
            .result("value")
            .result("lower")
            .result("upper")
            .real("change");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- responseHistory
// ---------------------------------------------------------------------------------------------------

class ResponseHistoryQuery final : public ResultQuery
{
public:
    ResponseHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const std::set<std::int64_t> wanted = idSet(context, "ids");
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<std::int32_t> cycle, id, subcase;
            std::vector<std::string> label, type;
            std::vector<double> value;
            std::vector<std::int16_t> provider;
            ItemColumns items;
            const DesignResponse* first = nullptr;
            for (const DesignResponse& item : data.responses())
            {
                if (item.set != set || (!wanted.empty() && !wanted.contains(item.id)))
                    continue;
                first = first != nullptr ? first : &item;
                cycle.push_back(item.cycle);
                id.push_back(item.id);
                subcase.push_back(item.subcase);
                label.push_back(item.label);
                type.push_back(item.type);
                value.push_back(item.value);
                provider.push_back(sources.of(item.source));
                items.add(item.itemId, item.component, item.layer);
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycle.size());
            block.ints("designCycle", std::move(cycle));
            block.ints("id", std::move(id));
            block.texts("label", std::move(label));
            block.texts("responseType", std::move(type));
            block.ints("subcase", std::move(subcase));
            block.reals("value", std::move(value));
            items.fill(block);
            block.providers(std::move(provider));
            describeBlock(block, context, set, *first);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "responseHistory";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "The responses of an optimisation at each design cycle: identification number, label, type, subcase and "
            "value. "
            "The definitions of the responses (`responses`) have no value: `value` is null for them; the values "
            "come from the responses a solver gives in memory. `ids` keeps some responses. itemId, component and "
            "layer tell apart the items of a response that has several (the grid point, element or mode, the "
            "component or item code, the ply): from the arrays of the same names a solver gives, or from the "
            "definitions of the responses (itemId and itemCode); null when not given.";
        info.parameters = {parameter("ids", ParameterKind::IdList, "None", "the responses to list")};
        info.defaultSort = "designCycle, id";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("id")
            .dictionary("label")
            .dictionary("responseType")
            .int32("subcase")
            .result("value")
            .int32("itemId")
            .int32("component")
            .int32("layer");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- constraintHistory
// -------------------------------------------------------------------------------------------------

class ConstraintHistoryQuery final : public ResultQuery
{
public:
    ConstraintHistoryQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const bool onlyViolated = context.arguments().boolOr("onlyViolated", false);
        const bool onlyActive = context.arguments().boolOr("onlyActive", false);
        const double tolerance = context.arguments().realOr("activeTolerance", 1.0e-3);
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<std::int32_t> cycle, constraintSet, response, subcase;
            std::vector<double> value, lower, upper, violation;
            std::vector<std::uint8_t> violated, active;
            std::vector<std::int16_t> provider;
            const DesignConstraint* first = nullptr;
            for (const DesignConstraint& item : data.constraints())
            {
                if (item.set != set)
                    continue;
                const bool isViolated = item.violation > 0.0;
                const bool isActive = !std::isnan(item.violation) && item.violation >= -tolerance;
                if ((onlyViolated && !isViolated) || (onlyActive && !isActive))
                    continue;
                first = first != nullptr ? first : &item;
                cycle.push_back(item.cycle);
                constraintSet.push_back(item.constraintSet);
                response.push_back(item.response);
                subcase.push_back(item.subcase);
                value.push_back(item.value);
                lower.push_back(item.lower);
                upper.push_back(item.upper);
                violation.push_back(item.violation);
                violated.push_back(isViolated ? 1 : 0);
                active.push_back(isActive ? 1 : 0);
                provider.push_back(sources.of(item.source));
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycle.size());
            block.ints("designCycle", std::move(cycle));
            block.ints("constraintSet", std::move(constraintSet));
            block.ints("responseId", std::move(response));
            block.ints("subcase", std::move(subcase));
            block.reals("value", std::move(value));
            block.reals("lower", std::move(lower));
            block.reals("upper", std::move(upper));
            block.reals("violation", std::move(violation));
            block.flags("isViolated", std::move(violated));
            block.flags("isActive", std::move(active));
            block.providers(std::move(provider));
            describeBlock(block, context, set, *first);
            accumulator.add(std::move(block));
        }
        if (accumulator.empty())
            return emptyTable(info().defaultColumns());
        return accumulator.finish(context, info());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "constraintHistory";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "The constraints at each design cycle: the value of the response, its bounds, and the violation normalised "
            "by the "
            "bound that is the closest to being violated (`(value - upper) / |upper|` above the upper bound, `(lower - "
            "value) / "
            "|lower|` under the lower one; positive when violated, negative when there is margin). `isActive` is true "
            "when "
            "the violation is above `-activeTolerance`: the constraint is at its bound or beyond it. The constraints "
            "come from "
            "the responses a solver gives in memory with bounds; the design history reports only the most critical "
            "constraint "
            "of each "
            "iteration (its normalised value, `lower` and `upper` null, `responseId` its row in the constraint "
            "table).";
        info.parameters = {parameter("onlyViolated", ParameterKind::Bool, "False", "keep the violated constraints"),
                           parameter("onlyActive", ParameterKind::Bool, "False", "keep the active constraints"),
                           parameter("activeTolerance", ParameterKind::Real, "0.001",
                                     "how close to its bound a constraint is active")};
        info.defaultSort = "designCycle, constraintSet, responseId";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("constraintSet")
            .int32("responseId")
            .int32("subcase")
            .result("value")
            .result("lower");
        columns.result("upper").real("violation").flag("isViolated").flag("isActive");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignHistoryQuery>());
}

void addDesignVariableHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignVariableHistoryQuery>());
}

void addResponseHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ResponseHistoryQuery>());
}

void addConstraintHistoryQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConstraintHistoryQuery>());
}

} // namespace oor::query
