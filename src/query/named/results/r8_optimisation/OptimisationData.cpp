#include "OptimisationData.hpp"

#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

double scaleOf(double bound)
{
    return std::fabs(bound) > 1.0e-9 ? std::fabs(bound) : 1.0;
}

const std::vector<DesignVariable> NO_VARIABLES;

// The number of rows and of columns of a 2-D array (the columns are 1 for a vector).
std::pair<std::size_t, std::size_t> shapeOf(const results::ResultArray& array)
{
    const std::size_t rows = array.shape.empty() ? 0 : array.shape[0];
    const std::size_t columns = array.shape.size() > 1 ? array.shape[1] : 1;
    return {rows, columns};
}

// The optional arrays that say which item of a response a row is (itemId, component, layer), 0 where absent.
struct ItemArrays
{
    std::vector<std::int32_t> itemId;
    std::vector<std::int32_t> component;
    std::vector<std::int32_t> layer;
};

ItemArrays itemArraysOf(const results::NamedResultData& data, std::size_t rows)
{
    const auto read = [&](const char* name)
    {
        std::vector<std::int32_t> values;
        if (const results::ResultArray* array = data.find(name))
            values = arrayInts(*array);
        values.resize(rows, 0);
        return values;
    };
    return ItemArrays{read("itemId"), read("component"), read("layer")};
}

} // namespace

void ItemColumns::add(std::int32_t itemId, std::int32_t component, std::int32_t layer)
{
    m_itemId.push_back(itemId);
    m_component.push_back(component);
    m_layer.push_back(layer);
}

void ItemColumns::fill(RowBlock& block)
{
    const auto valid = [](const std::vector<std::int32_t>& values)
    {
        std::vector<std::uint8_t> result;
        result.reserve(values.size());
        for (const std::int32_t value : values)
            result.push_back(value != 0 ? 1 : 0);
        return result;
    };
    std::vector<std::uint8_t> itemValid = valid(m_itemId);
    std::vector<std::uint8_t> componentValid = valid(m_component);
    std::vector<std::uint8_t> layerValid = valid(m_layer);
    block.ints("itemId", std::move(m_itemId), std::move(itemValid));
    block.ints("component", std::move(m_component), std::move(componentValid));
    block.ints("layer", std::move(m_layer), std::move(layerValid));
}

double normalisedViolation(double value, double lower, double upper, double* bound)
{
    double worst = -std::numeric_limits<double>::infinity();
    double which = NOT_A_NUMBER;
    if (std::isfinite(upper))
    {
        const double violation = (value - upper) / scaleOf(upper);
        if (violation > worst)
        {
            worst = violation;
            which = upper;
        }
    }
    if (std::isfinite(lower))
    {
        const double violation = (lower - value) / scaleOf(lower);
        if (violation > worst)
        {
            worst = violation;
            which = lower;
        }
    }
    if (bound != nullptr)
        *bound = which;
    return std::isfinite(worst) ? worst : NOT_A_NUMBER;
}

std::string noSensitivitiesMessage(const std::string& query)
{
    return query +
           " needs the sensitivities of the responses to the design variables, and the result sets hold none: a "
           "solver gives them as a result of the kind `sensitivities`";
}

OptimisationData::OptimisationData(const ResultQueryContext& context) : m_cycleFilter(context.selection().designCycles)
{
    readVariables(context);
    readCycles(context);
    readResponses(context);
    readSensitivities(context);
    deriveConstraints(context);
    std::set<std::size_t> sets;
    for (const DesignCycle& item : m_cycles)
        sets.insert(item.set);
    for (const DesignResponse& item : m_responses)
        sets.insert(item.set);
    for (const DesignSensitivity& item : m_sensitivities)
        sets.insert(item.set);
    for (const auto& item : m_variables)
        sets.insert(item.first);
    m_setsWithData.assign(sets.begin(), sets.end());
}

bool OptimisationData::keepCycle(std::int32_t cycle) const
{
    if (!m_cycleFilter)
        return true;
    return std::find(m_cycleFilter->begin(), m_cycleFilter->end(), cycle) != m_cycleFilter->end();
}

const std::vector<DesignVariable>& OptimisationData::variables(std::size_t set) const
{
    const auto found = m_variables.find(set);
    return found == m_variables.end() ? NO_VARIABLES : found->second;
}

const DesignVariable* OptimisationData::variable(std::size_t set, std::int32_t id) const
{
    for (const DesignVariable& item : variables(set))
        if (item.id == id)
            return &item;
    return nullptr;
}

std::int32_t OptimisationData::lastCycle(std::size_t set) const
{
    std::int32_t last = -1;
    for (const DesignCycle& item : m_cycles)
        if (item.set == set)
            last = std::max(last, item.cycle);
    for (const DesignResponse& item : m_responses)
        if (item.set == set)
            last = std::max(last, item.cycle);
    return last;
}

std::int32_t OptimisationData::firstCycle(std::size_t set) const
{
    std::int32_t first = std::numeric_limits<std::int32_t>::max();
    for (const DesignCycle& item : m_cycles)
        if (item.set == set)
            first = std::min(first, item.cycle);
    for (const DesignResponse& item : m_responses)
        if (item.set == set)
            first = std::min(first, item.cycle);
    return first == std::numeric_limits<std::int32_t>::max() ? -1 : first;
}

const DesignCycle* OptimisationData::cycle(std::size_t set, std::int32_t number) const
{
    for (const DesignCycle& item : m_cycles)
        if (item.set == set && item.cycle == number)
            return &item;
    return nullptr;
}

void OptimisationData::readVariables(const ResultQueryContext& context)
{
    for (const ResultQueryContext::NamedRef& ref : context.named("designVariables"))
    {
        if (m_variables.contains(ref.set))
            continue;
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* id = data->find("id");
        const results::ResultArray* lower = data->find("lower");
        const results::ResultArray* upper = data->find("upper");
        const results::ResultText* label = data->findText("label");
        if (id == nullptr)
            continue;
        const std::vector<std::int32_t> ids = arrayInts(*id);
        const std::vector<double> lowers =
            lower != nullptr ? arrayDoubles(*lower) : std::vector<double>(ids.size(), NOT_A_NUMBER);
        const std::vector<double> uppers =
            upper != nullptr ? arrayDoubles(*upper) : std::vector<double>(ids.size(), NOT_A_NUMBER);
        std::vector<DesignVariable> variables;
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            DesignVariable variable;
            variable.id = ids[row];
            variable.label = label != nullptr && row < label->size() ? label->at(row) : std::string();
            variable.lower = row < lowers.size() ? lowers[row] : NOT_A_NUMBER;
            variable.upper = row < uppers.size() ? uppers[row] : NOT_A_NUMBER;
            variables.push_back(std::move(variable));
        }
        m_variables[ref.set] = std::move(variables);
    }
}

void OptimisationData::readCycles(const ResultQueryContext& context)
{
    std::set<std::size_t> fromHistory;
    for (const ResultQueryContext::NamedRef& ref : context.named("designHistory"))
    {
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* iteration = data->find("designIteration");
        const results::ResultArray* objective = data->find("objectiveFinal");
        const results::ResultArray* constraint = data->find("maxConstraint");
        const results::ResultArray* constraintRow = data->find("maxConstraintRow");
        const results::ResultArray* result = data->find("convergenceResult");
        const results::ResultArray* values = data->find("designVariableValues");
        const results::ResultText* convergence = data->findText("convergence");
        if (iteration == nullptr || objective == nullptr)
            continue;
        fromHistory.insert(ref.set);
        const std::vector<std::int32_t> cycles = arrayInts(*iteration);
        const std::vector<double> objectives = arrayDoubles(*objective);
        const std::vector<double> constraints =
            constraint != nullptr ? arrayDoubles(*constraint) : std::vector<double>();
        const std::vector<std::int32_t> rows =
            constraintRow != nullptr ? arrayInts(*constraintRow) : std::vector<std::int32_t>();
        const std::vector<std::int32_t> results = result != nullptr ? arrayInts(*result) : std::vector<std::int32_t>();
        const std::vector<double> all = values != nullptr ? arrayDoubles(*values) : std::vector<double>();
        const std::size_t width = values != nullptr ? shapeOf(*values).second : 0;
        const std::vector<DesignVariable>& variables = this->variables(ref.set);
        for (std::size_t row = 0; row < cycles.size(); ++row)
        {
            if (!keepCycle(cycles[row]))
                continue;
            DesignCycle item;
            item.set = ref.set;
            item.analysis = context.sets()[ref.set].analysis;
            item.cycle = cycles[row];
            item.objective = row < objectives.size() ? objectives[row] : NOT_A_NUMBER;
            item.maxConstraint = row < constraints.size() ? constraints[row] : NOT_A_NUMBER;
            item.maxConstraintRow = row < rows.size() ? rows[row] : 0;
            if (row < results.size())
            {
                item.convergenceKnown = true;
                item.converged = results[row] == 1 || results[row] == 2;
            }
            if (convergence != nullptr && row < convergence->size())
                item.convergence = convergence->at(row);
            for (std::size_t column = 0; column < width && row * width + column < all.size(); ++column)
            {
                const std::int32_t id =
                    column < variables.size() ? variables[column].id : static_cast<std::int32_t>(column + 1);
                item.values[id] = all[row * width + column];
            }
            item.source = "design history";
            item.status = statusOfNamed(*ref.result);
            item.precision = precisionOfNamed(*data);
            m_cycles.push_back(std::move(item));
        }
    }
    // the design variable history for the sets that have no iteration history
    for (const ResultQueryContext::NamedRef& ref : context.named("designVariableHistory"))
    {
        if (fromHistory.contains(ref.set))
            continue;
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* objective = data->find("objective");
        const results::ResultArray* constraint = data->find("maxConstraint");
        const results::ResultArray* ids = data->find("designVariableIds");
        const results::ResultArray* values = data->find("designVariableValues");
        if (objective == nullptr)
            continue;
        const std::vector<double> objectives = arrayDoubles(*objective);
        const std::vector<double> constraints =
            constraint != nullptr ? arrayDoubles(*constraint) : std::vector<double>();
        const std::vector<std::int32_t> variableIds = ids != nullptr ? arrayInts(*ids) : std::vector<std::int32_t>();
        const std::vector<double> all = values != nullptr ? arrayDoubles(*values) : std::vector<double>();
        const std::size_t width = values != nullptr ? shapeOf(*values).second : 0;
        if (m_variables.find(ref.set) == m_variables.end() && !variableIds.empty())
        {
            std::vector<DesignVariable> variables;
            for (const std::int32_t id : variableIds)
            {
                DesignVariable variable;
                variable.id = id;
                variables.push_back(variable);
            }
            m_variables[ref.set] = std::move(variables);
        }
        for (std::size_t row = 0; row < objectives.size(); ++row)
        {
            if (!keepCycle(static_cast<std::int32_t>(row)))
                continue;
            DesignCycle item;
            item.set = ref.set;
            item.analysis = context.sets()[ref.set].analysis;
            item.cycle = static_cast<std::int32_t>(row);
            item.objective = objectives[row];
            item.maxConstraint = row < constraints.size() ? constraints[row] : NOT_A_NUMBER;
            for (std::size_t column = 0;
                 column < width && column < variableIds.size() && row * width + column < all.size(); ++column)
                item.values[variableIds[column]] = all[row * width + column];
            item.source = "design variable history";
            item.status = statusOfNamed(*ref.result);
            item.precision = precisionOfNamed(*data);
            m_cycles.push_back(std::move(item));
        }
    }
    std::stable_sort(m_cycles.begin(), m_cycles.end(), [](const DesignCycle& a, const DesignCycle& b)
                     { return std::tie(a.set, a.cycle) < std::tie(b.set, b.cycle); });
}

void OptimisationData::readResponses(const ResultQueryContext& context)
{
    std::set<std::size_t> withValues;
    for (const ResultQueryContext::NamedRef& ref : context.named("responseValues"))
    {
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* cycle = data->find("designCycle");
        const results::ResultArray* id = data->find("id");
        const results::ResultArray* subcase = data->find("subcase");
        const results::ResultArray* value = data->find("value");
        const results::ResultArray* lower = data->find("lower");
        const results::ResultArray* upper = data->find("upper");
        const results::ResultArray* set = data->find("constraintSet");
        const results::ResultArray* objective = data->find("isObjective");
        const results::ResultText* label = data->findText("label");
        const results::ResultText* type = data->findText("responseType");
        if (cycle == nullptr || id == nullptr || value == nullptr)
            continue;
        const ItemArrays items = itemArraysOf(*data, arrayInts(*id).size());
        withValues.insert(ref.set);
        const std::vector<std::int32_t> cycles = arrayInts(*cycle);
        const std::vector<std::int32_t> ids = arrayInts(*id);
        const std::vector<std::int32_t> subcases =
            subcase != nullptr ? arrayInts(*subcase) : std::vector<std::int32_t>(ids.size(), 0);
        const std::vector<double> values = arrayDoubles(*value);
        const std::vector<double> lowers =
            lower != nullptr ? arrayDoubles(*lower) : std::vector<double>(ids.size(), NOT_A_NUMBER);
        const std::vector<double> uppers =
            upper != nullptr ? arrayDoubles(*upper) : std::vector<double>(ids.size(), NOT_A_NUMBER);
        const std::vector<std::int32_t> sets =
            set != nullptr ? arrayInts(*set) : std::vector<std::int32_t>(ids.size(), 0);
        const std::vector<std::int32_t> flags =
            objective != nullptr ? arrayInts(*objective) : std::vector<std::int32_t>(ids.size(), 0);
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            if (!keepCycle(cycles[row]))
                continue;
            DesignResponse item;
            item.set = ref.set;
            item.analysis = context.sets()[ref.set].analysis;
            item.cycle = cycles[row];
            item.id = ids[row];
            item.subcase = subcases[row];
            item.label = label != nullptr && row < label->size() ? label->at(row) : std::string();
            item.type = type != nullptr && row < type->size() ? type->at(row) : std::string();
            item.value = values[row];
            item.lower = lowers[row];
            item.upper = uppers[row];
            item.constraintSet = sets[row];
            item.isObjective = flags[row] != 0;
            item.itemId = items.itemId[row];
            item.component = items.component[row];
            item.layer = items.layer[row];
            item.source = "response values (in memory)";
            item.status = statusOfNamed(*ref.result);
            item.precision = precisionOfNamed(*data);
            m_responses.push_back(std::move(item));
        }
    }
    // the definitions of the responses (the named result `responses`): no value
    std::map<std::size_t, std::int32_t> position;
    for (const ResultQueryContext::NamedRef& ref : context.named("responses"))
    {
        if (withValues.contains(ref.set))
            continue;
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* id = data->find("id");
        const results::ResultArray* subcase = data->find("subcase");
        const results::ResultText* label = data->findText("label");
        const results::ResultText* type = data->findText("typeName");
        const results::ResultArray* itemId = data->find("itemId");
        const results::ResultArray* itemCode = data->find("itemCode");
        if (id == nullptr)
            continue;
        const std::int32_t cycle =
            ref.result->meta().designCycle >= 0 ? ref.result->meta().designCycle : position[ref.set];
        ++position[ref.set];
        if (!keepCycle(cycle))
            continue;
        const std::vector<std::int32_t> ids = arrayInts(*id);
        const std::vector<std::int32_t> subcases =
            subcase != nullptr ? arrayInts(*subcase) : std::vector<std::int32_t>(ids.size(), 0);
        const std::vector<std::int32_t> itemIds = itemId != nullptr ? arrayInts(*itemId) : std::vector<std::int32_t>();
        const std::vector<std::int32_t> itemCodes =
            itemCode != nullptr ? arrayInts(*itemCode) : std::vector<std::int32_t>();
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            DesignResponse item;
            item.itemId = row < itemIds.size() ? itemIds[row] : 0;
            item.component = row < itemCodes.size() ? itemCodes[row] : 0;
            item.set = ref.set;
            item.analysis = context.sets()[ref.set].analysis;
            item.cycle = cycle;
            item.id = ids[row];
            item.subcase = subcases[row];
            item.label = label != nullptr && row < label->size() ? label->at(row) : std::string();
            item.type = type != nullptr && row < type->size() ? type->at(row) : std::string();
            item.source = "response definitions";
            item.status = statusOfNamed(*ref.result);
            item.precision = precisionOfNamed(*data);
            m_responses.push_back(std::move(item));
        }
    }
    std::stable_sort(m_responses.begin(), m_responses.end(), [](const DesignResponse& a, const DesignResponse& b)
                     { return std::tie(a.set, a.cycle) < std::tie(b.set, b.cycle); });
}

void OptimisationData::readSensitivities(const ResultQueryContext& context)
{
    for (const ResultQueryContext::NamedRef& ref : context.named("sensitivities"))
    {
        const std::shared_ptr<const results::NamedResultData> data = ref.result->data(context.decode());
        const results::ResultArray* cycle = data->find("designCycle");
        const results::ResultArray* response = data->find("responseId");
        const results::ResultArray* subcase = data->find("subcase");
        const results::ResultArray* variable = data->find("designVariable");
        const results::ResultArray* sensitivity = data->find("sensitivity");
        if (cycle == nullptr || response == nullptr || variable == nullptr || sensitivity == nullptr)
            continue;
        const ItemArrays items = itemArraysOf(*data, arrayInts(*cycle).size());
        const std::vector<std::int32_t> cycles = arrayInts(*cycle);
        const std::vector<std::int32_t> responses = arrayInts(*response);
        const std::vector<std::int32_t> subcases =
            subcase != nullptr ? arrayInts(*subcase) : std::vector<std::int32_t>(cycles.size(), 0);
        const std::vector<std::int32_t> variables = arrayInts(*variable);
        const std::vector<double> values = arrayDoubles(*sensitivity);
        for (std::size_t row = 0; row < cycles.size(); ++row)
        {
            if (!keepCycle(cycles[row]))
                continue;
            DesignSensitivity item;
            item.set = ref.set;
            item.cycle = cycles[row];
            item.response = responses[row];
            item.subcase = subcases[row];
            item.variable = variables[row];
            item.value = values[row];
            item.itemId = items.itemId[row];
            item.component = items.component[row];
            item.layer = items.layer[row];
            item.source = "sensitivities (in memory)";
            item.status = statusOfNamed(*ref.result);
            item.precision = precisionOfNamed(*data);
            m_sensitivities.push_back(std::move(item));
        }
    }
}

void OptimisationData::deriveConstraints(const ResultQueryContext& context)
{
    std::set<std::size_t> fromResponses;
    for (const DesignResponse& response : m_responses)
    {
        if (!std::isfinite(response.lower) && !std::isfinite(response.upper))
            continue;
        if (std::isnan(response.value))
            continue;
        fromResponses.insert(response.set);
        DesignConstraint item;
        item.set = response.set;
        item.analysis = response.analysis;
        item.cycle = response.cycle;
        item.constraintSet = response.constraintSet;
        item.response = response.id;
        item.subcase = response.subcase;
        item.value = response.value;
        item.lower = response.lower;
        item.upper = response.upper;
        item.violation = normalisedViolation(response.value, response.lower, response.upper, &item.bound);
        item.source = "constraints on the response values";
        item.status = response.status;
        item.precision = response.precision;
        m_constraints.push_back(std::move(item));
    }
    // the most critical constraint of each iteration, the only one the design history reports
    for (const DesignCycle& cycle : m_cycles)
    {
        if (fromResponses.contains(cycle.set) || std::isnan(cycle.maxConstraint))
            continue;
        DesignConstraint item;
        item.set = cycle.set;
        item.analysis = cycle.analysis;
        item.cycle = cycle.cycle;
        item.response = cycle.maxConstraintRow;
        item.value = cycle.maxConstraint;
        item.violation = cycle.maxConstraint;
        item.source = "most critical constraint of the design history";
        item.status = cycle.status;
        item.precision = cycle.precision;
        m_constraints.push_back(std::move(item));
    }
    (void)context;
}

} // namespace oor::query
