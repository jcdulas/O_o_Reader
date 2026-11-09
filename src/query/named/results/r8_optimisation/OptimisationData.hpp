#pragma once

#include "../NamedTables.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace oor::query
{

// What the queries of the family R8 read: the optimisation results of the result sets (the named results
// `designVariables`, `designHistory`, `designVariableHistory`, `responses`, `responseValues` and `sensitivities`), put
// in the same few shapes. A value the results do not hold (the response values, the bounds of the constraints) is NaN.

struct DesignVariable
{
    std::int32_t id = 0;
    std::string label;
    double lower = std::nan("");
    double upper = std::nan("");
};

// One design cycle of one result set: the objective, the largest constraint, the convergence and the value of each
// variable.
struct DesignCycle
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t cycle = 0;
    double objective = std::nan("");
    double maxConstraint = std::nan(""); // as the file says: negative when no constraint is violated
    std::int32_t maxConstraintRow = 0;   // the row of that constraint in the constraint table (0: not said)
    bool convergenceKnown = false;
    bool converged = false;
    std::string convergence;
    std::map<std::int32_t, double> values; // by design variable ID
    std::string source;
    std::string status;
    Precision precision = Precision::Double;
};

// One response at one design cycle.
struct DesignResponse
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t cycle = 0;
    std::int32_t id = 0;
    std::int32_t subcase = 0;
    std::string label;
    std::string type;
    double value = std::nan("");
    double lower = std::nan("");
    double upper = std::nan("");
    std::int32_t constraintSet = 0;
    bool isObjective = false;
    // The item of a response that has several (a companion package's P1.6): the grid point, element or mode, the
    // component or item code, the ply; 0 when not given.
    std::int32_t itemId = 0;
    std::int32_t component = 0;
    std::int32_t layer = 0;
    std::string source;
    std::string status;
    Precision precision = Precision::Double;
};

struct DesignSensitivity
{
    std::size_t set = 0;
    std::int32_t cycle = 0;
    std::int32_t response = 0;
    std::int32_t subcase = 0;
    std::int32_t variable = 0;
    double value = 0.0;
    std::int32_t itemId = 0; // the item of the response, as in DesignResponse
    std::int32_t component = 0;
    std::int32_t layer = 0;
    std::string source;
    std::string status;
    Precision precision = Precision::Double;
};

// The item columns of the responses and sensitivities (itemId, component, layer), null where 0.
class ItemColumns
{
public:
    void add(std::int32_t itemId, std::int32_t component, std::int32_t layer);
    void fill(RowBlock& block);

private:
    std::vector<std::int32_t> m_itemId;
    std::vector<std::int32_t> m_component;
    std::vector<std::int32_t> m_layer;
};

// A constraint of a design cycle: a response with a bound, or the most critical constraint of an iteration that a file
// reports.
struct DesignConstraint
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t cycle = 0;
    std::int32_t constraintSet = 0;
    std::int32_t response = 0;
    std::int32_t subcase = 0;
    double value = std::nan("");
    double lower = std::nan("");
    double upper = std::nan("");
    double violation = std::nan(""); // normalised, positive when violated
    double bound = std::nan("");     // the bound that is the closest to being violated
    std::string source;
    std::string status;
    Precision precision = Precision::Double;
};

// The normalised violation of a bound: (value - upper) / |upper| above the upper bound, (lower - value) / |lower| under
// the lower one, the larger of the two; positive when the constraint is violated. A bound of zero is normalised by one.
[[nodiscard]] double normalisedViolation(double value, double lower, double upper, double* bound = nullptr);

class OptimisationData
{
public:
    explicit OptimisationData(const ResultQueryContext& context);

    [[nodiscard]] bool empty() const noexcept
    {
        return m_cycles.empty() && m_responses.empty() && m_sensitivities.empty() && m_variables.empty();
    }
    [[nodiscard]] const std::vector<DesignCycle>& cycles() const noexcept { return m_cycles; }
    [[nodiscard]] const std::vector<DesignResponse>& responses() const noexcept { return m_responses; }
    [[nodiscard]] const std::vector<DesignSensitivity>& sensitivities() const noexcept { return m_sensitivities; }
    [[nodiscard]] const std::vector<DesignConstraint>& constraints() const noexcept { return m_constraints; }
    // The variables of a result set, in the order of their table.
    [[nodiscard]] const std::vector<DesignVariable>& variables(std::size_t set) const;
    [[nodiscard]] const DesignVariable* variable(std::size_t set, std::int32_t id) const;
    // The last design cycle of a set (-1 when it has none).
    [[nodiscard]] std::int32_t lastCycle(std::size_t set) const;
    [[nodiscard]] std::int32_t firstCycle(std::size_t set) const;
    // The cycle of a set, or null.
    [[nodiscard]] const DesignCycle* cycle(std::size_t set, std::int32_t number) const;
    // True when the result sets give no sensitivity (the queries that need them say so).
    [[nodiscard]] bool hasSensitivities() const noexcept { return !m_sensitivities.empty(); }
    [[nodiscard]] const std::vector<std::size_t>& sets() const noexcept { return m_setsWithData; }

private:
    void readVariables(const ResultQueryContext& context);
    void readCycles(const ResultQueryContext& context);
    void readResponses(const ResultQueryContext& context);
    void readSensitivities(const ResultQueryContext& context);
    void deriveConstraints(const ResultQueryContext& context);
    [[nodiscard]] bool keepCycle(std::int32_t cycle) const;

    std::optional<std::vector<std::int64_t>> m_cycleFilter;
    std::map<std::size_t, std::vector<DesignVariable>> m_variables;
    std::vector<DesignCycle> m_cycles;
    std::vector<DesignResponse> m_responses;
    std::vector<DesignSensitivity> m_sensitivities;
    std::vector<DesignConstraint> m_constraints;
    std::vector<std::size_t> m_setsWithData;
};

// The providers a query's rows come from, by name, for the provenance (one per table the rows were read from).
class SourceIndex
{
public:
    explicit SourceIndex(RowAccumulator& accumulator) : m_accumulator(accumulator) {}
    [[nodiscard]] std::int16_t of(const std::string& name, bool approximate = false)
    {
        const auto found = m_index.find(name);
        if (found != m_index.end())
            return found->second;
        const std::int16_t index = m_accumulator.addProvider(name, approximate);
        m_index.emplace(name, index);
        return index;
    }

private:
    RowAccumulator& m_accumulator;
    std::map<std::string, std::int16_t> m_index;
};

// The wording of a query that needs the sensitivities and finds none: what the result sets hold and how to give them.
[[nodiscard]] std::string noSensitivitiesMessage(const std::string& query);

} // namespace oor::query
