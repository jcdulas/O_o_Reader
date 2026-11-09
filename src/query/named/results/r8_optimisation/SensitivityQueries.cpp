#include "OptimisationData.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r8_optimisation/OptimisationQueries.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

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

double normalisationOf(double bound)
{
    return std::fabs(bound) > 1.0e-9 ? std::fabs(bound) : 1.0;
}

// The range a variable moves in, to make a sensitivity dimensionless: the distance between its bounds, else its value,
// else 1.
double rangeOf(const DesignVariable* variable, double value)
{
    if (variable != nullptr && std::isfinite(variable->lower) && std::isfinite(variable->upper) &&
        variable->upper > variable->lower)
        return variable->upper - variable->lower;
    if (std::isfinite(value) && value != 0.0)
        return std::fabs(value);
    return 1.0;
}

bool atBound(double value, double bound, double range, double tolerance)
{
    if (!std::isfinite(bound) || !std::isfinite(value))
        return false;
    return std::fabs(value - bound) <= tolerance * std::max(range, 1.0e-12);
}

// The sensitivities of a set at a design cycle, found by response, subcase and variable. A sensitivity stored with no
// subcase (0) serves every subcase of the response, and a constraint with no subcase takes the first one the response
// has.
class SensitivityIndex
{
public:
    SensitivityIndex(const OptimisationData& data, std::size_t set, std::int32_t cycle)
    {
        for (const DesignSensitivity& item : data.sensitivities())
        {
            if (item.set != set || item.cycle != cycle)
                continue;
            m_exact.emplace(std::make_tuple(item.response, item.subcase, item.variable), &item);
            m_any.emplace(std::make_pair(item.response, item.variable), &item);
            m_variables.insert(item.variable);
        }
    }
    [[nodiscard]] const DesignSensitivity* find(std::int32_t response, std::int32_t subcase,
                                                std::int32_t variable) const
    {
        auto found = m_exact.find(std::make_tuple(response, subcase, variable));
        if (found != m_exact.end())
            return found->second;
        if (subcase != 0)
        {
            found = m_exact.find(std::make_tuple(response, 0, variable));
            if (found != m_exact.end())
                return found->second;
            return nullptr;
        }
        const auto any = m_any.find(std::make_pair(response, variable));
        return any == m_any.end() ? nullptr : any->second;
    }
    [[nodiscard]] const std::set<std::int32_t>& variables() const noexcept { return m_variables; }

private:
    std::map<std::tuple<std::int32_t, std::int32_t, std::int32_t>, const DesignSensitivity*> m_exact;
    std::map<std::pair<std::int32_t, std::int32_t>, const DesignSensitivity*> m_any;
    std::set<std::int32_t> m_variables;
};

// The design cycle a query works on: the one the caller named (an error when the set has nothing at it), else the last
// of the set.
std::int32_t cycleOf(const ResultQueryContext& context, const OptimisationData& data, std::size_t set)
{
    if (!context.arguments().has("designCycle"))
        return data.lastCycle(set);
    const auto cycle = static_cast<std::int32_t>(context.arguments().integerOr("designCycle", 0));
    bool known = data.cycle(set, cycle) != nullptr;
    for (const DesignResponse& item : data.responses())
        known = known || (item.set == set && item.cycle == cycle);
    for (const DesignSensitivity& item : data.sensitivities())
        known = known || (item.set == set && item.cycle == cycle);
    if (!known)
        throw QueryError("the design cycle " + std::to_string(cycle) + " is not one of the result set '" +
                         context.sets()[set].analysis + "' (its cycles run from " +
                         std::to_string(data.firstCycle(set)) + " to " + std::to_string(data.lastCycle(set)) + ")");
    return cycle;
}

void requireSensitivities(const ResultQueryContext& context, const OptimisationData& data, const std::string& query)
{
    if (!data.hasSensitivities() && context.selection().onMissing == OnMissing::Raise && !data.empty())
        throw QuantityUnavailableError(noSensitivitiesMessage(query));
}

template <typename Row>
void describeBlock(RowBlock& block, const ResultQueryContext& context, std::size_t set, const Row& first)
{
    block.origin(set, context.sets()[set].analysis, -1);
    block.precision(first.precision);
    block.validation(first.status);
}

// ---- sensitivitiesTable
// ------------------------------------------------------------------------------------------------

class SensitivitiesTableQuery final : public ResultQuery
{
public:
    SensitivitiesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const std::set<std::int64_t> responses = idSet(context, "responseIds");
        const std::set<std::int64_t> variables = idSet(context, "designVariableIds");
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<std::int32_t> cycle, response, subcase, variable;
            std::vector<double> sensitivity;
            std::vector<std::int16_t> provider;
            ItemColumns items;
            const DesignSensitivity* first = nullptr;
            for (const DesignSensitivity& item : data.sensitivities())
            {
                if (item.set != set || (!responses.empty() && !responses.contains(item.response)) ||
                    (!variables.empty() && !variables.contains(item.variable)))
                    continue;
                first = first != nullptr ? first : &item;
                cycle.push_back(item.cycle);
                response.push_back(item.response);
                subcase.push_back(item.subcase);
                variable.push_back(item.variable);
                sensitivity.push_back(item.value);
                provider.push_back(sources.of(item.source));
                items.add(item.itemId, item.component, item.layer);
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycle.size());
            block.ints("designCycle", std::move(cycle));
            block.ints("responseId", std::move(response));
            block.ints("subcase", std::move(subcase));
            block.ints("designVariable", std::move(variable));
            block.reals("sensitivity", std::move(sensitivity));
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
        info.name = "sensitivitiesTable";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "The sensitivity of each response to each design variable at each design cycle, one row for each response, "
            "subcase "
            "and variable: the derivatives a solver gives (the named result `sensitivities`). `designCycles` keeps "
            "some cycles. itemId, "
            "component and layer tell apart the items of a response that has several (the grid point, element or "
            "mode, the component, the ply): from the arrays a solver gives; "
            "null when not given.";
        info.parameters = {
            parameter("responseIds", ParameterKind::IdList, "None", "the responses to list"),
            parameter("designVariableIds", ParameterKind::IdList, "None", "the design variables to list")};
        info.defaultSort = "designCycle, responseId, designVariable";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("responseId")
            .int32("subcase")
            .int32("designVariable")
            .result("sensitivity")
            .int32("itemId")
            .int32("component")
            .int32("layer");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- constraintDrivers
// -------------------------------------------------------------------------------------------------

class ConstraintDriversQuery final : public ResultQuery
{
public:
    ConstraintDriversQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const std::int64_t top = context.arguments().integerOr("top", 5);
        if (top < 1)
            throw QueryError("top is a number of variables, at least 1");
        const bool onlyActive = context.arguments().boolOr("onlyActive", true);
        const double tolerance = context.arguments().realOr("activeTolerance", 1.0e-3);
        requireSensitivities(context, data, "constraintDrivers");
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            const std::int32_t cycle = cycleOf(context, data, set);
            const SensitivityIndex index(data, set, cycle);
            const DesignCycle* record = data.cycle(set, cycle);
            std::vector<std::int32_t> cycles, constraintSet, response, subcase, rank, variable;
            std::vector<double> violation, sensitivity, normalised, value;
            std::vector<std::uint8_t> bound;
            std::vector<std::int16_t> provider;
            const DesignSensitivity* first = nullptr;
            for (const DesignConstraint& constraint : data.constraints())
            {
                if (constraint.set != set || constraint.cycle != cycle || std::isnan(constraint.violation))
                    continue;
                if (onlyActive && constraint.violation < -tolerance)
                    continue;
                struct Driver
                {
                    std::int32_t variable;
                    const DesignSensitivity* item;
                    double normalised;
                };
                std::vector<Driver> drivers;
                const double scale = normalisationOf(constraint.bound);
                for (const std::int32_t id : index.variables())
                {
                    const DesignSensitivity* item = index.find(constraint.response, constraint.subcase, id);
                    if (item == nullptr)
                        continue;
                    double current = NOT_A_NUMBER;
                    if (record != nullptr)
                        if (const auto found = record->values.find(id); found != record->values.end())
                            current = found->second;
                    drivers.push_back({id, item, item->value * rangeOf(data.variable(set, id), current) / scale});
                }
                std::stable_sort(drivers.begin(), drivers.end(), [](const Driver& a, const Driver& b)
                                 { return std::fabs(a.normalised) > std::fabs(b.normalised); });
                for (std::size_t position = 0; position < drivers.size() && position < static_cast<std::size_t>(top);
                     ++position)
                {
                    const Driver& driver = drivers[position];
                    first = first != nullptr ? first : driver.item;
                    double current = NOT_A_NUMBER;
                    if (record != nullptr)
                        if (const auto found = record->values.find(driver.variable); found != record->values.end())
                            current = found->second;
                    const DesignVariable* definition = data.variable(set, driver.variable);
                    const double range = rangeOf(definition, current);
                    cycles.push_back(cycle);
                    constraintSet.push_back(constraint.constraintSet);
                    response.push_back(constraint.response);
                    subcase.push_back(constraint.subcase);
                    violation.push_back(constraint.violation);
                    rank.push_back(static_cast<std::int32_t>(position + 1));
                    variable.push_back(driver.variable);
                    sensitivity.push_back(driver.item->value);
                    normalised.push_back(driver.normalised);
                    value.push_back(current);
                    bound.push_back(definition != nullptr && (atBound(current, definition->lower, range, 1.0e-6) ||
                                                              atBound(current, definition->upper, range, 1.0e-6))
                                        ? 1
                                        : 0);
                    provider.push_back(sources.of(driver.item->source));
                }
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycles.size());
            block.ints("designCycle", std::move(cycles));
            block.ints("constraintSet", std::move(constraintSet));
            block.ints("responseId", std::move(response));
            block.ints("subcase", std::move(subcase));
            block.reals("violation", std::move(violation));
            block.ints("rank", std::move(rank));
            block.ints("designVariable", std::move(variable));
            block.reals("sensitivity", std::move(sensitivity));
            block.reals("normalisedSensitivity", std::move(normalised));
            block.reals("variableValue", std::move(value));
            block.flags("variableAtBound", std::move(bound));
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
        info.name = "constraintDrivers";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "For each constraint of a design cycle, the design variables that influence it most: the `top` variables "
            "ranked by "
            "the size of their normalised sensitivity, the sensitivity times the range of the variable (the distance "
            "between its "
            "bounds, else its value) over the bound of the constraint. `designCycle` defaults to the last cycle. Only "
            "the "
            "active constraints (violation above `-activeTolerance`) are listed unless `onlyActive` is false. Needs "
            "the "
            "sensitivities and the constraints (the responses with bounds that a solver gives in memory).";
        info.parameters = {parameter("top", ParameterKind::Integer, "5", "the number of variables for each constraint"),
                           parameter("onlyActive", ParameterKind::Bool, "True", "list the active constraints only"),
                           parameter("designCycle", ParameterKind::Integer, "None", "the cycle (the last by default)"),
                           parameter("activeTolerance", ParameterKind::Real, "0.001",
                                     "how close to its bound a constraint is active")};
        info.defaultSort = "constraintSet, responseId, rank";
        ColumnListBuilder columns;
        columns.int32("designCycle").int32("constraintSet").int32("responseId").int32("subcase").real("violation");
        columns.int32("rank")
            .int32("designVariable")
            .result("sensitivity")
            .real("normalisedSensitivity")
            .real("variableValue");
        columns.flag("variableAtBound");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- variableImportance
// ------------------------------------------------------------------------------------------------

class VariableImportanceQuery final : public ResultQuery
{
public:
    VariableImportanceQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const double tolerance = context.arguments().realOr("tolerance", 1.0e-9);
        constexpr double ACTIVE = 1.0e-3;
        requireSensitivities(context, data, "variableImportance");
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            const std::int32_t cycle = cycleOf(context, data, set);
            const SensitivityIndex index(data, set, cycle);
            const DesignCycle* record = data.cycle(set, cycle);
            // the objective: the response flagged as such, else the objective of the cycle with the sensitivities of
            // the response
            const DesignResponse* objective = nullptr;
            for (const DesignResponse& response : data.responses())
                if (response.set == set && response.cycle == cycle && response.isObjective)
                {
                    objective = &response;
                    break;
                }
            std::vector<std::int32_t> variables;
            for (const DesignVariable& variable : data.variables(set))
                variables.push_back(variable.id);
            for (const std::int32_t id : index.variables())
                if (std::find(variables.begin(), variables.end(), id) == variables.end())
                    variables.push_back(id);
            std::vector<std::int32_t> cycles, count;
            std::vector<std::int32_t> id;
            std::vector<std::string> label;
            std::vector<double> value, objectiveSensitivity, normalisedObjective, maxConstraint;
            std::vector<std::uint8_t> ineffective;
            std::vector<std::int16_t> provider;
            const DesignSensitivity* first = nullptr;
            for (const std::int32_t variableId : variables)
            {
                const DesignVariable* definition = data.variable(set, variableId);
                double current = NOT_A_NUMBER;
                if (record != nullptr)
                    if (const auto found = record->values.find(variableId); found != record->values.end())
                        current = found->second;
                const double range = rangeOf(definition, current);
                double objectiveValue = NOT_A_NUMBER;
                double normalisedValue = NOT_A_NUMBER;
                const DesignSensitivity* source = nullptr;
                if (objective != nullptr)
                    if (const DesignSensitivity* item = index.find(objective->id, objective->subcase, variableId))
                    {
                        source = item;
                        objectiveValue = item->value;
                        const double reference =
                            std::isnan(objective->value) && record != nullptr ? record->objective : objective->value;
                        normalisedValue =
                            std::isnan(reference) ? NOT_A_NUMBER : item->value * range / normalisationOf(reference);
                    }
                std::int32_t active = 0;
                double worst = NOT_A_NUMBER;
                for (const DesignConstraint& constraint : data.constraints())
                {
                    if (constraint.set != set || constraint.cycle != cycle || std::isnan(constraint.violation) ||
                        constraint.violation < -ACTIVE)
                        continue;
                    const DesignSensitivity* item = index.find(constraint.response, constraint.subcase, variableId);
                    if (item == nullptr)
                        continue;
                    source = source != nullptr ? source : item;
                    if (std::fabs(item->value) > tolerance)
                        ++active;
                    const double size = std::fabs(item->value * range / normalisationOf(constraint.bound));
                    worst = std::isnan(worst) ? size : std::max(worst, size);
                }
                bool effective = false;
                for (const DesignSensitivity& item : data.sensitivities())
                    if (item.set == set && item.cycle == cycle && item.variable == variableId &&
                        std::fabs(item.value) > tolerance)
                    {
                        effective = true;
                        source = source != nullptr ? source : &item;
                    }
                if (source == nullptr)
                    source = data.sensitivities().empty() ? nullptr : &data.sensitivities().front();
                if (source == nullptr)
                    continue;
                first = first != nullptr ? first : source;
                cycles.push_back(cycle);
                id.push_back(variableId);
                label.push_back(definition != nullptr ? definition->label : std::string());
                value.push_back(current);
                objectiveSensitivity.push_back(objectiveValue);
                normalisedObjective.push_back(normalisedValue);
                count.push_back(active);
                maxConstraint.push_back(worst);
                ineffective.push_back(effective ? 0 : 1);
                provider.push_back(sources.of(source->source));
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycles.size());
            block.ints("designCycle", std::move(cycles));
            block.ints("designVariable", std::move(id));
            block.texts("label", std::move(label));
            block.reals("value", std::move(value));
            block.reals("objectiveSensitivity", std::move(objectiveSensitivity));
            block.reals("normalisedObjectiveSensitivity", std::move(normalisedObjective));
            block.ints("activeConstraintCount", std::move(count));
            block.reals("maxConstraintSensitivity", std::move(maxConstraint));
            block.flags("isIneffective", std::move(ineffective));
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
        info.name = "variableImportance";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "What each design variable does at a design cycle (the last by default): the sensitivity of the objective "
            "to it, "
            "normalised by the range of the variable over the objective, the number of active constraints it "
            "influences "
            "(sensitivity above `tolerance`), the largest normalised sensitivity of an active constraint to it, and "
            "whether "
            "it has no sensitivity above `tolerance` on any response (`isIneffective`). The objective is the response "
            "a "
            "solver flags as such; the sensitivities and the constraints come from the tables of the result sets "
            "(`sensitivitiesTable`, `constraintHistory`).";
        info.parameters = {parameter("designCycle", ParameterKind::Integer, "None", "the cycle (the last by default)"),
                           parameter("tolerance", ParameterKind::Real, "1e-09",
                                     "the sensitivity under which a variable has no effect")};
        info.defaultSort = "designVariable";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("designVariable")
            .dictionary("label")
            .real("value")
            .real("objectiveSensitivity");
        columns.real("normalisedObjectiveSensitivity").int32("activeConstraintCount").real("maxConstraintSensitivity");
        columns.flag("isIneffective");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- variablesAtBounds
// ------------------------------------------------------------------------------------------------

class VariablesAtBoundsQuery final : public ResultQuery
{
public:
    VariablesAtBoundsQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const OptimisationData data(context);
        const double tolerance = context.arguments().realOr("tolerance", 1.0e-6);
        RowAccumulator accumulator;
        SourceIndex sources(accumulator);
        for (const std::size_t set : data.sets())
        {
            std::vector<std::int32_t> cycle, id;
            std::vector<std::string> label, side;
            std::vector<double> value, boundValue;
            std::vector<std::int16_t> provider;
            const DesignCycle* first = nullptr;
            for (const DesignCycle& item : data.cycles())
            {
                if (item.set != set)
                    continue;
                for (const DesignVariable& variable : data.variables(set))
                {
                    const auto found = item.values.find(variable.id);
                    if (found == item.values.end())
                        continue;
                    const double range = rangeOf(&variable, found->second);
                    for (const char* name : {"lower", "upper"})
                    {
                        const double bound = std::string(name) == "lower" ? variable.lower : variable.upper;
                        if (!atBound(found->second, bound, range, tolerance))
                            continue;
                        first = first != nullptr ? first : &item;
                        cycle.push_back(item.cycle);
                        id.push_back(variable.id);
                        label.push_back(variable.label);
                        value.push_back(found->second);
                        side.emplace_back(name);
                        boundValue.push_back(bound);
                        provider.push_back(sources.of(item.source));
                        // a variable whose bounds are equal is reported once
                        if (variable.lower == variable.upper)
                            break;
                    }
                }
            }
            if (first == nullptr)
                continue;
            RowBlock block(cycle.size());
            block.ints("designCycle", std::move(cycle));
            block.ints("designVariable", std::move(id));
            block.texts("label", std::move(label));
            block.reals("value", std::move(value));
            block.texts("bound", std::move(side));
            block.reals("boundValue", std::move(boundValue));
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
        info.name = "variablesAtBounds";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "The design variables that sit on one of their bounds at a design cycle: the value, which bound (`lower` "
            "or `upper`) "
            "and its value. A variable is on a bound when it is within `tolerance` times the range between its bounds "
            "of it. "
            "`designCycles` keeps some cycles.";
        info.parameters = {
            parameter("tolerance", ParameterKind::Real, "1e-06", "the distance to a bound, relative to the range")};
        info.defaultSort = "designCycle, designVariable";
        ColumnListBuilder columns;
        columns.int32("designCycle")
            .int32("designVariable")
            .dictionary("label")
            .real("value")
            .dictionary("bound")
            .real("boundValue");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

// ---- linearisedPrediction
// ----------------------------------------------------------------------------------------------

class LinearisedPredictionQuery final : public ResultQuery
{
public:
    LinearisedPredictionQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const std::vector<std::pair<std::string, double>> changes = context.arguments().values("values");
        if (changes.empty())
            throw QueryError("linearisedPrediction needs `values`: a dict {design variable: new value}");
        const OptimisationData data(context);
        requireSensitivities(context, data, "linearisedPrediction");
        RowAccumulator accumulator;
        // a first order estimate: marked approximate
        const std::int16_t provider =
            accumulator.addProvider("sensitivities times the change of the design variables", true);
        for (const std::size_t set : data.sets())
        {
            const std::int32_t cycle = cycleOf(context, data, set);
            const DesignCycle* record = data.cycle(set, cycle);
            if (record == nullptr)
                continue;
            // the variables by ID or by label
            std::map<std::int32_t, double> target;
            for (const auto& [key, value] : changes)
            {
                std::int32_t id = 0;
                bool found = false;
                try
                {
                    std::size_t used = 0;
                    const long parsed = std::stol(key, &used);
                    if (used == key.size())
                    {
                        id = static_cast<std::int32_t>(parsed);
                        found = data.variable(set, id) != nullptr || record->values.contains(id);
                    }
                }
                catch (const std::exception&)
                {
                }
                if (!found)
                    for (const DesignVariable& variable : data.variables(set))
                        if (variable.label == key)
                        {
                            id = variable.id;
                            found = true;
                        }
                if (!found)
                {
                    std::string known;
                    for (const DesignVariable& variable : data.variables(set))
                        known += (known.empty() ? "" : ", ") + std::to_string(variable.id) +
                                 (variable.label.empty() ? "" : " (" + variable.label + ")");
                    throw QueryError("the design variable '" + key + "' is not one of the result set '" +
                                     context.sets()[set].analysis + "'; it has: " + known);
                }
                if (!record->values.contains(id))
                    throw QueryError("the design cycle " + std::to_string(cycle) +
                                     " does not give the value of the design variable " + std::to_string(id) +
                                     ": the change cannot be computed");
                target[id] = value;
            }
            const SensitivityIndex index(data, set, cycle);
            std::vector<std::int32_t> response, subcase;
            std::vector<std::string> label;
            std::vector<std::uint8_t> isObjective, predictedViolated, violatedValid;
            std::vector<double> value, predicted, change, lower, upper, violation;
            const DesignResponse* first = nullptr;
            for (const DesignResponse& item : data.responses())
            {
                if (item.set != set || item.cycle != cycle)
                    continue;
                first = first != nullptr ? first : &item;
                double delta = 0.0;
                bool known = !std::isnan(item.value);
                for (const auto& [variable, newValue] : target)
                {
                    const DesignSensitivity* sensitivity = index.find(item.id, item.subcase, variable);
                    if (sensitivity == nullptr)
                    {
                        // a response with no sensitivity to a variable does not change with it only when the table has
                        // the response at all: no row for the response, no prediction
                        bool hasResponse = false;
                        for (const std::int32_t other : index.variables())
                            hasResponse = hasResponse || index.find(item.id, item.subcase, other) != nullptr;
                        known = known && hasResponse;
                        continue;
                    }
                    delta += sensitivity->value * (newValue - record->values.at(variable));
                }
                response.push_back(item.id);
                subcase.push_back(item.subcase);
                label.push_back(item.label);
                isObjective.push_back(item.isObjective ? 1 : 0);
                value.push_back(item.value);
                predicted.push_back(known ? item.value + delta : NOT_A_NUMBER);
                change.push_back(known ? delta : NOT_A_NUMBER);
                lower.push_back(item.lower);
                upper.push_back(item.upper);
                const bool bounded = std::isfinite(item.lower) || std::isfinite(item.upper);
                const double predictedViolation =
                    known && bounded ? normalisedViolation(item.value + delta, item.lower, item.upper) : NOT_A_NUMBER;
                violation.push_back(predictedViolation);
                predictedViolated.push_back(!std::isnan(predictedViolation) && predictedViolation > 0.0 ? 1 : 0);
                violatedValid.push_back(std::isnan(predictedViolation) ? 0 : 1);
            }
            if (first == nullptr)
                continue;
            RowBlock block(response.size());
            block.ints("responseId", std::move(response));
            block.texts("label", std::move(label));
            block.ints("subcase", std::move(subcase));
            block.flags("isObjective", std::move(isObjective));
            block.reals("value", std::move(value));
            block.reals("predicted", std::move(predicted));
            block.reals("change", std::move(change));
            block.reals("lower", std::move(lower));
            block.reals("upper", std::move(upper));
            block.reals("predictedViolation", std::move(violation));
            block.flags("isPredictedViolated", std::move(predictedViolated), std::move(violatedValid));
            block.provider(provider);
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
        info.name = "linearisedPrediction";
        info.family = "R8";
        info.resultQuery = true;
        info.description =
            "A first order estimate of the responses for new values of some design variables: each response of the "
            "design cycle "
            "(the last by default) plus the sum of its sensitivities times the change of the variables that `values` "
            "gives (a "
            "dict `{design variable ID or label: new value}`; a variable it does not name keeps its value). Exact for "
            "a response "
            "that is linear in the variables and an estimate otherwise: the provenance marks it approximate. "
            "`predictedViolation` "
            "is the normalised violation of the bounds of the response at the predicted value. The sensitivities are "
            "used as "
            "the solver gives them.";
        info.parameters = {
            parameter("values", ParameterKind::Values, "required", "the new values: {design variable: value}"),
            parameter("designCycle", ParameterKind::Integer, "None", "the cycle (the last by default)")};
        info.defaultSort = "responseId, subcase";
        ColumnListBuilder columns;
        columns.int32("responseId")
            .dictionary("label")
            .int32("subcase")
            .flag("isObjective")
            .real("value")
            .result("predicted");
        columns.result("change").real("lower").real("upper").result("predictedViolation").flag("isPredictedViolated");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addSensitivitiesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SensitivitiesTableQuery>());
}

void addConstraintDriversQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConstraintDriversQuery>());
}

void addVariableImportanceQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<VariableImportanceQuery>());
}

void addVariablesAtBoundsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<VariablesAtBoundsQuery>());
}

void addLinearisedPredictionQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LinearisedPredictionQuery>());
}

} // namespace oor::query
