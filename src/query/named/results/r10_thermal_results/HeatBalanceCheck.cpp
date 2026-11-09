#include "../NamedTables.hpp"
#include "oor/analysis/validation/engine/ValidationSupport.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/results/r10_thermal_results/ThermalResultQueries.hpp"
#include "oor/query/named/topology/t11_thermal/HeatLoads.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/providers/quantities/QuantityProvider.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// A total of a quantity at one step of one subcase of one result set.
struct StepTotal
{
    std::size_t set = 0;
    std::string analysis;
    std::int32_t subcase = 0;
    std::int32_t designCycle = -1;
    results::StepKind stepKind = results::StepKind::None;
    double step = NOT_A_NUMBER;
    double value = 0.0;
    Precision precision = Precision::Single;
    std::string status;
    std::int16_t provider = 0;
};

class TotalSet
{
public:
    void beginProvider(std::int16_t index, const ProviderInfo& info)
    {
        m_current = index;
        if (m_providers.size() <= static_cast<std::size_t>(index))
            m_providers.resize(static_cast<std::size_t>(index) + 1);
        m_providers[static_cast<std::size_t>(index)] = info;
    }
    [[nodiscard]] std::int64_t rowCount() const noexcept { return static_cast<std::int64_t>(m_rows.size()); }
    bool add(StepTotal&& total)
    {
        const auto key = std::make_tuple(total.set, total.subcase, total.designCycle, stepKey(total.step));
        if (!m_keys.emplace(key, m_rows.size()).second)
            return false;
        total.provider = m_current;
        m_rows.push_back(std::move(total));
        return true;
    }
    // Adds the value to the total of the same set, subcase, design cycle and step, or starts one.
    void accumulate(const StepTotal& total)
    {
        const auto key = std::make_tuple(total.set, total.subcase, total.designCycle, stepKey(total.step));
        const auto found = m_keys.find(key);
        if (found != m_keys.end())
        {
            m_rows[found->second].value += total.value;
            return;
        }
        m_keys.emplace(key, m_rows.size());
        m_rows.push_back(total);
    }
    [[nodiscard]] bool covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle, double step) const
    {
        return m_keys.contains(std::make_tuple(set, subcase, designCycle, stepKey(step)));
    }
    [[nodiscard]] const std::vector<StepTotal>& rows() const noexcept { return m_rows; }
    [[nodiscard]] const std::vector<ProviderInfo>& providers() const noexcept { return m_providers; }

private:
    std::vector<StepTotal> m_rows;
    std::map<std::tuple<std::size_t, std::int32_t, std::int32_t, std::uint64_t>, std::size_t> m_keys;
    std::vector<ProviderInfo> m_providers;
    std::int16_t m_current = 0;
};

using HeatProvider = QuantityProvider<int, TotalSet>;

// The sum over the rows of a table, for each step, of the components `names` (all added).
void addTotals(const ResultQueryContext& context, const TableRef& ref, const std::vector<std::string>& names,
               TotalSet& sink)
{
    const results::ResultTableMeta& meta = ref.table->meta();
    const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
    const TableValues values(*decoded);
    std::vector<int> component;
    for (const std::string& name : names)
        if (const int found = values.component(name); found >= 0)
            component.push_back(found);
    if (component.empty() || values.isComplex())
        return;
    const bool stepped = meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                         meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
    for (std::size_t step = 0; step < values.steps(); ++step)
    {
        StepTotal total;
        total.set = ref.set;
        total.analysis = context.sets()[ref.set].analysis;
        total.subcase = meta.subcase;
        total.designCycle = meta.designCycle;
        total.stepKind = stepped ? meta.stepKind : results::StepKind::None;
        total.step = stepped ? decoded->steps[step] : NOT_A_NUMBER;
        total.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
        total.status = std::string(results::resultStatusName(meta.status));
        for (std::size_t row = 0; row < values.rows(); ++row)
            for (const int column : component)
                total.value += values.real(step, row, static_cast<std::size_t>(column));
        sink.accumulate(total);
    }
}

// Moves the totals of `local` that `sink` does not have yet into it; returns how many.
std::int64_t transfer(const TotalSet& local, TotalSet& sink)
{
    std::int64_t moved = 0;
    for (const StepTotal& total : local.rows())
    {
        if (sink.covers(total.set, total.subcase, total.designCycle, total.step))
            continue;
        StepTotal copy = total;
        if (sink.add(std::move(copy)))
            ++moved;
    }
    return moved;
}

// ---- the applied heat
// ----------------------------------------------------------------------------------------------------------

class AppliedHeatTableProvider final : public HeatProvider
{
public:
    [[nodiscard]] std::string name() const override { return "applied-load table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const int&, TotalSet& sink) const override
    {
        std::vector<TableRef> tables = context.tables(results::ResultKind::ThermalLoads, false);
        if (tables.empty())
            return {0, "the result file has no applied heat load table (output of OLOAD was not requested)"};
        sortTables(tables);
        TotalSet local;
        for (const TableRef& ref : tables)
            addTotals(context, ref, {"heatLoad"}, local);
        Outcome outcome;
        outcome.rows = transfer(local, sink);
        return outcome;
    }
};

class SurfaceElementHeatProvider final : public HeatProvider
{
public:
    [[nodiscard]] std::string name() const override { return "applied heat of the surface elements"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const int&, TotalSet& sink) const override
    {
        Outcome outcome;
        bool anyTable = false;
        TotalSet local;
        for (const char* name : {"HeatSurfaceSide", "HeatSurfaceNodes", "HeatSurfaceProperty"})
        {
            std::vector<TableRef> tables = context.elementTables(results::ResultKind::HeatFlows, name, false);
            sortTables(tables);
            for (const TableRef& ref : tables)
            {
                anyTable = true;
                addTotals(context, ref, {"applied"}, local); // the surface elements are added together
            }
        }
        outcome.rows = transfer(local, sink);
        if (outcome.rows == 0)
            outcome.reason = anyTable ? "every step was already given"
                                      : "the result file has no heat flow table of surface elements";
        return outcome;
    }
};

// The heat load cards (QBDY1, QBDY2, QBDY3, QHBDY, QVOL, QVECT) the LOAD command of a subcase selects: the power they
// put into their targets, added (`heatLoadsTable`). It is the way of a steady-state analysis whose result file has no
// table of the applied heat: the load of a transient analysis is a function of time (TLOADi) that the cards do not give
// as a number.
class HeatLoadCardsProvider final : public HeatProvider
{
public:
    [[nodiscard]] std::string name() const override { return "heat load cards of the model"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const int&, TotalSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "the model is not given: the heat load cards are not read"};
        // The steady-state subcases the result tables have (a table with a step belongs to a transient analysis).
        struct Case
        {
            std::size_t set = 0;
            std::int32_t subcase = 0;
            std::int32_t designCycle = -1;
            std::string status;
            Precision precision = Precision::Single;
        };
        std::vector<Case> cases;
        bool transient = false;
        const auto note = [&](const TableRef& ref)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const bool stepped =
                meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
            if (stepped)
            {
                transient = true;
                return;
            }
            for (const Case& known : cases)
            {
                if (known.set == ref.set && known.subcase == meta.subcase && known.designCycle == meta.designCycle)
                    return;
            }
            cases.push_back(Case{ref.set, meta.subcase, meta.designCycle,
                                 std::string(results::resultStatusName(meta.status)), Precision::Double});
        };
        for (const TableRef& ref : context.tables(results::ResultKind::Temperatures, false))
            note(ref);
        for (const TableRef& ref : context.tables(results::ResultKind::ConstraintHeat, false))
            note(ref);
        for (const char* element : {"HeatSurfaceSide", "HeatSurfaceNodes", "HeatSurfaceProperty"})
        {
            for (const TableRef& ref : context.elementTables(results::ResultKind::HeatFlows, element, false))
                note(ref);
        }
        if (cases.empty())
            return {
                0,
                transient
                    ? "the analysis is transient: its heat loads are functions of time (TLOADi), which the "
                      "cards do not give as one number"
                    : "the result file has no temperature, constraint heat or heat flow table to say which subcase to "
                      "take the loads of"};
        const model::CaseControl& caseControl = context.modelPointer()->control().caseControl;
        const model::CaseLineages lineages(caseControl);
        TotalSet local;
        std::string reason;
        for (const Case& item : cases)
        {
            const model::Subcase* subcase = nullptr;
            for (const model::Subcase* candidate : analysis::analysisSubcases(caseControl))
            {
                if (candidate->id == item.subcase)
                    subcase = candidate;
            }
            const auto set = subcase != nullptr ? analysis::effectiveInteger(lineages, *subcase, model::LOAD_SELECTION)
                                                : std::nullopt;
            if (!set)
            {
                reason =
                    "subcase " + std::to_string(item.subcase) + " has no LOAD command in the Case Control of the model";
                continue;
            }
            const std::vector<HeatLoad> loads = computeHeatLoads(
                context.modelPointer(), context.exec(), std::vector<base::EntityId>{static_cast<base::EntityId>(*set)});
            if (loads.empty())
            {
                reason = "the set " + std::to_string(*set) + " of subcase " + std::to_string(item.subcase) +
                         " has no heat load card (QBDY1, QBDY2, QBDY3, QHBDY, QVOL, QVECT)";
                continue;
            }
            double total = 0.0;
            for (const HeatLoad& load : loads)
                total += load.power;
            if (!std::isfinite(total))
            {
                reason = "a heat load of the set " + std::to_string(*set) + " has no power: its target has no geometry";
                continue;
            }
            StepTotal value;
            value.set = item.set;
            value.analysis = context.sets()[item.set].analysis;
            value.subcase = item.subcase;
            value.designCycle = item.designCycle;
            value.value = total;
            value.precision = item.precision;
            value.status = item.status;
            local.accumulate(value);
        }
        Outcome outcome;
        outcome.rows = transfer(local, sink);
        if (outcome.rows == 0)
            outcome.reason = reason.empty() ? "every step was already given" : reason;
        return outcome;
    }
};

const ProviderChain<int, TotalSet>& appliedHeatChain()
{
    static const ProviderChain<int, TotalSet> CHAIN = []
    {
        ProviderChain<int, TotalSet> chain("applied heat");
        chain.add(std::make_unique<AppliedHeatTableProvider>());
        chain.add(std::make_unique<SurfaceElementHeatProvider>());
        chain.add(std::make_unique<HeatLoadCardsProvider>());
        return chain;
    }();
    return CHAIN;
}

// heatBalanceCheck: the heat that is applied, the heat that leaves by convection, radiation and through the
// constraints, and what is left.
class HeatBalanceCheckQuery final : public ResultQuery
{
public:
    HeatBalanceCheckQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const double tolerance = context.arguments().realOr("tolerance", 1.0e-3);
        if (!(tolerance >= 0.0))
            throw QueryError("tolerance is a relative residual, not negative");
        TotalSet applied;
        const std::vector<ProviderAttempt> attempts = appliedHeatChain().resolve(context, 0, applied);
        // the other terms: sums over the tables that exist
        TotalSet convected;
        TotalSet radiated;
        TotalSet constraint;
        const auto sumFlows = [&](const std::vector<std::string>& names, TotalSet& into)
        {
            for (const char* element : {"HeatSurfaceSide", "HeatSurfaceNodes", "HeatSurfaceProperty"})
            {
                std::vector<TableRef> tables = context.elementTables(results::ResultKind::HeatFlows, element, false);
                sortTables(tables);
                for (const TableRef& ref : tables)
                {
                    TotalSet local;
                    addTotals(context, ref, names, local);
                    for (const StepTotal& total : local.rows())
                        into.accumulate(total); // tables of different element types add up
                }
            }
        };
        sumFlows({"FreeConvection", "forcedConvection"}, convected);
        sumFlows({"radiation"}, radiated);
        {
            std::vector<TableRef> tables = context.tables(results::ResultKind::ConstraintHeat, false);
            sortTables(tables);
            for (const TableRef& ref : tables)
                addTotals(context, ref, {"heatFlow"}, constraint);
        }
        if (applied.rowCount() == 0)
        {
            if (context.selection().onMissing == OnMissing::Raise)
                throw QuantityUnavailableError(describeAttempts("the applied heat", attempts));
            return emptyTable(info().defaultColumns());
        }
        const auto find = [](const TotalSet& set, const StepTotal& like) -> const StepTotal*
        {
            for (const StepTotal& row : set.rows())
                if (row.set == like.set && row.subcase == like.subcase && row.designCycle == like.designCycle &&
                    stepKey(row.step) == stepKey(like.step))
                    return &row;
            return nullptr;
        };
        RowAccumulator accumulator;
        std::vector<std::int16_t> providers;
        for (const ProviderInfo& provider : applied.providers())
            providers.push_back(accumulator.addProvider(provider.name, provider.approximate));
        std::map<std::tuple<std::int32_t, std::size_t, std::int32_t, int>, std::vector<const StepTotal*>> groups;
        for (const StepTotal& row : applied.rows())
            groups[{row.designCycle, row.set, row.subcase, static_cast<int>(row.stepKind)}].push_back(&row);
        for (const auto& [key, rows] : groups)
        {
            const std::size_t count = rows.size();
            std::vector<double> appliedValue, convectedValue, radiatedValue, constraintValue, residual, relative, steps;
            std::vector<std::uint8_t> balanced, convectedValid, radiatedValid, constraintValid;
            std::vector<std::string> source;
            std::vector<std::int16_t> provider;
            bool single = true;
            for (const StepTotal* row : rows)
            {
                const StepTotal* c = find(convected, *row);
                const StepTotal* r = find(radiated, *row);
                const StepTotal* k = find(constraint, *row);
                const double a = row->value;
                const double terms[3] = {c != nullptr ? c->value : 0.0, r != nullptr ? r->value : 0.0,
                                         k != nullptr ? k->value : 0.0};
                const double rest = a + terms[0] + terms[1] + terms[2];
                const double scale =
                    std::max({std::fabs(a), c != nullptr ? std::fabs(c->value) : 0.0,
                              r != nullptr ? std::fabs(r->value) : 0.0, k != nullptr ? std::fabs(k->value) : 0.0});
                appliedValue.push_back(a);
                convectedValue.push_back(c != nullptr ? c->value : NOT_A_NUMBER);
                radiatedValue.push_back(r != nullptr ? r->value : NOT_A_NUMBER);
                constraintValue.push_back(k != nullptr ? k->value : NOT_A_NUMBER);
                convectedValid.push_back(c != nullptr ? 1 : 0);
                residual.push_back(rest);
                relative.push_back(scale > 0.0 ? std::fabs(rest) / scale : NOT_A_NUMBER);
                balanced.push_back(scale > 0.0 ? (std::fabs(rest) / scale <= tolerance ? 1 : 0)
                                               : (rest == 0.0 ? 1 : 0));
                const ProviderInfo& way = applied.providers()[static_cast<std::size_t>(row->provider)];
                source.push_back(sourceText(way.name, way.approximate));
                provider.push_back(providers[static_cast<std::size_t>(row->provider)]);
                steps.push_back(row->step);
                single = single && row->precision == Precision::Single;
            }
            (void)convectedValid;
            RowBlock block(count);
            block.ints("subcase", std::vector<std::int32_t>(count, std::get<2>(key)));
            block.reals("applied", std::move(appliedValue));
            block.reals("convected", std::move(convectedValue));
            block.reals("radiated", std::move(radiatedValue));
            block.reals("constraint", std::move(constraintValue));
            block.reals("residual", std::move(residual));
            block.reals("relativeResidual", std::move(relative));
            block.flags("balanced", std::move(balanced));
            block.texts("appliedSource", std::move(source));
            block.providers(std::move(provider));
            const StepTotal& first = *rows.front();
            if (first.stepKind != results::StepKind::None)
                block.steps(first.stepKind, std::move(steps));
            block.origin(first.set, first.analysis, first.designCycle);
            block.precision(single ? Precision::Single : Precision::Double);
            block.validation(first.status);
            accumulator.add(std::move(block));
        }
        return sortedResult(accumulator.finish(context, info()), context.exec(), leadingKeys());
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "heatBalanceCheck";
        info.family = "R10";
        info.resultQuery = true;
        info.description =
            "The heat balance of a heat transfer analysis, for each subcase and step: the applied heat, the heat "
            "convected and "
            "radiated by the surface elements and the heat flowing through the constraints (each is null when the "
            "result file "
            "has no table of it), and their sum, the residual, which is zero when the heat that goes in comes out (the "
            "heat "
            "that leaves has the negative sign of the tables). `relativeResidual` is the residual over the largest of "
            "the "
            "terms; `balanced` is true when it is within `tolerance`. The applied heat comes from the applied-load "
            "table, else "
            "from the applied flux of the surface elements, else, for a steady-state subcase, from the heat load cards "
            "its LOAD "
            "command selects (the power of `heatLoadsTable`; `appliedSource` says which). A transient analysis "
            "stores heat in the materials, which is not a term "
            "here: only a "
            "steady-state analysis balances.";
        info.parameters = {
            parameter("tolerance", ParameterKind::Real, "0.001", "the relative residual that is accepted")};
        info.defaultSort = "subcase, [step]";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .results("applied convected radiated constraint residual")
            .real("relativeResidual");
        columns.flag("balanced").dictionary("appliedSource");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

QuantityInfo appliedHeatQuantity()
{
    return appliedHeatChain().info();
}

void addHeatBalanceCheckQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<HeatBalanceCheckQuery>());
}

} // namespace oor::query
