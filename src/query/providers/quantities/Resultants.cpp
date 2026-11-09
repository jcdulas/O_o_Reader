#include "oor/query/providers/quantities/Resultants.hpp"

#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/NamedQueryRegistry.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/providers/quantities/ElementRows.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

void ResultantSet::beginProvider(std::int16_t index, const ProviderInfo& info)
{
    m_current = index;
    if (m_providers.size() <= static_cast<std::size_t>(index))
        m_providers.resize(static_cast<std::size_t>(index) + 1);
    m_providers[static_cast<std::size_t>(index)] = info;
}

bool ResultantSet::add(ResultantRow&& row)
{
    const Key key{row.set, row.subcase, row.designCycle, stepKey(row.step)};
    if (m_keys.empty() || m_keys.back() < key)
        m_keys.push_back(key);
    else
    {
        const auto at = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        if (at != m_keys.end() && *at == key)
            return false;
        m_keys.insert(at, key);
    }
    row.provider = m_current;
    m_rows.push_back(std::move(row));
    return true;
}

bool ResultantSet::covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle, double step) const
{
    return std::binary_search(m_keys.begin(), m_keys.end(), Key{set, subcase, designCycle, stepKey(step)});
}

namespace
{

constexpr const char* const FORCE_NAMES[6] = {"t1", "t2", "t3", "r1", "r2", "r3"};
constexpr const char* const NODE_FORCE_NAMES[6] = {"f1", "f2", "f3", "m1", "m2", "m3"};

bool wants(const std::optional<std::vector<std::int64_t>>& list, std::int32_t id)
{
    return !list || std::find(list->begin(), list->end(), id) != list->end();
}

StepForces headerOf(const TableRef& ref, const ResultQueryContext& context, const results::ResultData& data,
                    std::size_t step)
{
    const results::ResultTableMeta& meta = ref.table->meta();
    StepForces forces;
    forces.set = ref.set;
    forces.analysis = context.sets()[ref.set].analysis;
    forces.subcase = meta.subcase;
    forces.designCycle = meta.designCycle;
    forces.stepKind = meta.stepKind;
    forces.step = meta.stepKind == results::StepKind::None || meta.stepKind == results::StepKind::LoadCombinationSet ||
                          step >= data.steps.size()
                      ? std::nan("")
                      : data.steps[step];
    forces.eigenvalue = step < data.eigenvalues.size() ? data.eigenvalues[step] : std::nan("");
    forces.precision = precisionOf(data);
    return forces;
}

} // namespace

std::vector<StepForces> nodalForcesOf(const ResultQueryContext& context, const TableRef& ref,
                                      const std::optional<std::vector<std::int64_t>>& nodes)
{
    std::vector<StepForces> found;
    const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
    const TableValues values(*decoded);
    if (values.isComplex())
        return found;
    int index[6];
    for (std::size_t component = 0; component < 6; ++component)
    {
        index[component] = values.component(FORCE_NAMES[component]);
        if (index[component] < 0)
            return found;
    }
    for (std::size_t step = 0; step < values.steps(); ++step)
    {
        StepForces forces = headerOf(ref, context, *decoded, step);
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            if (!wants(nodes, decoded->ids[row]))
                continue;
            // a scalar point or an extra point is no place in space: its value is not part of a force balance
            if (row < decoded->pointTypes.size() && decoded->pointTypes[row] != 1)
                continue;
            std::array<double, 6> value{};
            for (std::size_t component = 0; component < 6; ++component)
                value[component] = values.real(step, row, static_cast<std::size_t>(index[component]));
            forces.node.push_back(decoded->ids[row]);
            forces.force.push_back(value);
        }
        found.push_back(std::move(forces));
    }
    return found;
}

std::vector<StepForces> nodePointForcesOf(const ResultQueryContext& context, const TableRef& ref,
                                          const std::string& label,
                                          const std::optional<std::vector<std::int64_t>>& elements,
                                          const std::optional<std::vector<std::int64_t>>& nodes)
{
    std::vector<StepForces> found;
    const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
    const TableValues values(*decoded);
    if (values.isComplex())
        return found;
    int index[6];
    for (std::size_t component = 0; component < 6; ++component)
    {
        index[component] = values.component(NODE_FORCE_NAMES[component]);
        if (index[component] < 0)
            return found;
    }
    const results::ResultText* names = nullptr;
    for (const results::ResultText& text : decoded->texts)
        if (text.name == "elementName")
            names = &text;
    for (std::size_t step = 0; step < values.steps(); ++step)
    {
        StepForces forces = headerOf(ref, context, *decoded, step);
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            if (!wants(nodes, decoded->ids[row]))
                continue;
            const std::int32_t element = row < decoded->elementIds.size() ? decoded->elementIds[row] : 0;
            if (!label.empty())
            {
                if (names == nullptr || names->at(row) != label)
                    continue;
            }
            else
            {
                if (element <= 0 || !wants(elements, element))
                    continue;
            }
            std::array<double, 6> value{};
            for (std::size_t component = 0; component < 6; ++component)
                value[component] = values.real(step, row, static_cast<std::size_t>(index[component]));
            forces.node.push_back(decoded->ids[row]);
            forces.force.push_back(value);
        }
        found.push_back(std::move(forces));
    }
    return found;
}

ResultantRow resultantOf(const StepForces& forces, const std::array<double, 3>& point, ResultNodePositions& positions)
{
    ResultantRow row;
    row.set = forces.set;
    row.analysis = forces.analysis;
    row.subcase = forces.subcase;
    row.designCycle = forces.designCycle;
    row.stepKind = forces.stepKind;
    row.step = forces.step;
    row.eigenvalue = forces.eigenvalue;
    row.precision = forces.precision;
    row.count = static_cast<std::int64_t>(forces.node.size());
    std::array<double, 6> sum{};
    bool positioned = true;
    for (std::size_t index = 0; index < forces.node.size(); ++index)
    {
        const std::array<double, 6>& value = forces.force[index];
        for (std::size_t component = 0; component < 6; ++component)
            sum[component] += value[component];
        const std::array<double, 3> at = positions.position(forces.set, forces.node[index]);
        if (std::isnan(at[0]))
        {
            positioned = false;
            continue;
        }
        const double rx = at[0] - point[0];
        const double ry = at[1] - point[1];
        const double rz = at[2] - point[2];
        sum[3] += ry * value[2] - rz * value[1];
        sum[4] += rz * value[0] - rx * value[2];
        sum[5] += rx * value[1] - ry * value[0];
    }
    if (!positioned)
        for (std::size_t component = 3; component < 6; ++component)
            sum[component] = std::nan("");
    row.value = sum;
    return row;
}

std::array<double, 6> resultantInSystem(const ResultQueryContext& context, std::size_t set,
                                        const std::array<double, 6>& basic, std::int64_t system)
{
    if (system == 0)
        return basic;
    const model::Model* model = context.modelOf(set);
    if (model == nullptr)
        throw QueryError("the query '" + context.queryName() + "' was asked for the frame " + std::to_string(system) +
                         " and the results have no model to define it");
    analysis::CoordinateTransform transform(*model);
    const base::Mat3 axes = transform.frame(static_cast<base::EntityId>(system)).axes;
    const base::Vec3 force = axes.transposed() * base::Vec3{basic[0], basic[1], basic[2]};
    const base::Vec3 moment = axes.transposed() * base::Vec3{basic[3], basic[4], basic[5]};
    return {force.x, force.y, force.z, moment.x, moment.y, moment.z};
}

std::vector<std::int16_t> registerProviders(const ResultantSet& rows, RowAccumulator& accumulator)
{
    std::vector<std::int16_t> mapped;
    for (const ProviderInfo& info : rows.providers())
        mapped.push_back(accumulator.addProvider(info.name, info.approximate));
    return mapped;
}

void addResultantRows(const ResultQueryContext& context, const ResultantSet& rows, const ResultantRequest& request,
                      std::int64_t system, RowAccumulator& accumulator, const std::vector<std::int16_t>& providers,
                      const std::string& countColumn, const std::string& elementCountColumn)
{
    std::vector<std::size_t> order(rows.rows().size());
    for (std::size_t index = 0; index < order.size(); ++index)
        order[index] = index;
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right)
                     {
                         const ResultantRow& a = rows.rows()[left];
                         const ResultantRow& b = rows.rows()[right];
                         if (a.designCycle != b.designCycle)
                             return a.designCycle < b.designCycle;
                         if (a.set != b.set)
                             return a.set < b.set;
                         if (a.subcase != b.subcase)
                             return a.subcase < b.subcase;
                         return a.step < b.step || (std::isnan(b.step) && !std::isnan(a.step));
                     });
    // rows of one subcase of one result set go together in one block
    std::size_t begin = 0;
    while (begin < order.size())
    {
        const ResultantRow& first = rows.rows()[order[begin]];
        std::size_t end = begin;
        while (end < order.size())
        {
            const ResultantRow& row = rows.rows()[order[end]];
            if (row.set != first.set || row.subcase != first.subcase || row.designCycle != first.designCycle ||
                row.stepKind != first.stepKind || row.precision != first.precision)
                break;
            ++end;
        }
        const std::size_t count = end - begin;
        RowBlock block(count);
        std::vector<std::int32_t> subcase(count, first.subcase);
        std::array<std::vector<double>, 6> values;
        std::vector<std::int32_t> nodes;
        std::vector<std::int32_t> elementCounts;
        std::vector<double> steps;
        std::vector<double> eigenvalues;
        std::vector<std::int16_t> provider;
        std::vector<std::string> source;
        for (std::size_t position = begin; position < end; ++position)
        {
            const ResultantRow& row = rows.rows()[order[position]];
            const std::array<double, 6> expressed = resultantInSystem(context, row.set, row.value, system);
            for (std::size_t component = 0; component < 6; ++component)
                values[component].push_back(expressed[component]);
            nodes.push_back(static_cast<std::int32_t>(row.count));
            elementCounts.push_back(static_cast<std::int32_t>(row.elementCount));
            steps.push_back(row.step);
            eigenvalues.push_back(row.eigenvalue);
            provider.push_back(providers[static_cast<std::size_t>(row.provider)]);
            const ProviderInfo& info = rows.providers()[static_cast<std::size_t>(row.provider)];
            source.push_back(row.note.empty() ? sourceText(info.name, info.approximate)
                                              : sourceText(row.note, info.approximate));
        }
        block.ints("subcase", std::move(subcase));
        static const char* const NAMES[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
        for (std::size_t component = 0; component < 6; ++component)
            block.reals(NAMES[component], std::move(values[component]));
        block.reals("pointX", std::vector<double>(count, request.point[0]));
        block.reals("pointY", std::vector<double>(count, request.point[1]));
        block.reals("pointZ", std::vector<double>(count, request.point[2]));
        block.ints(countColumn, std::move(nodes));
        if (!elementCountColumn.empty())
            block.ints(elementCountColumn, std::move(elementCounts));
        block.texts("source", std::move(source));
        block.providers(std::move(provider));
        if (first.stepKind == results::StepKind::Time || first.stepKind == results::StepKind::Frequency ||
            first.stepKind == results::StepKind::Mode || first.stepKind == results::StepKind::LoadFactor)
        {
            const bool anyEigen =
                std::any_of(eigenvalues.begin(), eigenvalues.end(), [](double v) { return !std::isnan(v); });
            block.steps(first.stepKind, std::move(steps), anyEigen ? std::move(eigenvalues) : std::vector<double>());
        }
        block.origin(first.set, first.analysis, first.designCycle);
        block.precision(first.precision);
        accumulator.add(std::move(block));
        begin = end;
    }
}

namespace
{

// ---- the reactions
// ---------------------------------------------------------------------------------------------------

// Reactions, first way: the SPC force table of the result file.
class SinglePointConstrForceTableProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "SPC force table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        std::vector<TableRef> tables = context.resultantTables(results::ResultKind::SinglePointConstrForces);
        sortTables(tables);
        if (tables.empty())
            return {0,
                    "the result file has no SPC force table (output of SPCFORCES was not requested, or the subcases "
                    "asked for have none)"};
        ResultNodePositions positions(context);
        Outcome outcome;
        for (const TableRef& ref : tables)
            for (const StepForces& forces : nodalForcesOf(context, ref, request.nodeIds))
            {
                if (sink.covers(forces.set, forces.subcase, forces.designCycle, forces.step))
                    continue;
                if (sink.add(resultantOf(forces, request.point, positions)))
                    ++outcome.rows;
            }
        if (outcome.rows == 0)
            outcome.reason = "the table has no real values for the steps asked for";
        return outcome;
    }
};

// Reactions, second way: the singlePointConstrForce rows of the grid point force balance.
constexpr const char* SINGLE_POINT_CONSTR_FORCE_LABEL = "singlePointConstrForce";

class NodePointForceBalanceProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "grid point force balance (singlePointConstrForce)"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        std::vector<TableRef> tables = context.resultantTables(results::ResultKind::NodePointForces);
        sortTables(tables);
        if (tables.empty())
            return {0, "the result file has no grid point force table (output of GPFORCE was not requested)"};
        ResultNodePositions positions(context);
        Outcome outcome;
        bool anyLabel = false;
        for (const TableRef& ref : tables)
            for (const StepForces& forces :
                 nodePointForcesOf(context, ref, SINGLE_POINT_CONSTR_FORCE_LABEL, std::nullopt, request.nodeIds))
            {
                anyLabel = anyLabel || !forces.node.empty();
                if (sink.covers(forces.set, forces.subcase, forces.designCycle, forces.step))
                    continue;
                if (sink.add(resultantOf(forces, request.point, positions)))
                    ++outcome.rows;
            }
        if (outcome.rows == 0)
            outcome.reason = anyLabel ? "every step was already given"
                                      : "the grid point forces carry no singlePointConstrForce rows (no constrained "
                                        "grid point)";
        return outcome;
    }
};

// ---- the applied loads
// -------------------------------------------------------------------------------------------------

// Applied loads, first way: the applied-load table (OLOAD) of the result file.
class AppliedLoadTableProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "applied-load table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        std::vector<TableRef> tables = context.resultantTables(results::ResultKind::AppliedLoads);
        sortTables(tables);
        if (tables.empty())
            return {0, "the result file has no applied-load table (output of OLOAD was not requested)"};
        ResultNodePositions positions(context);
        Outcome outcome;
        for (const TableRef& ref : tables)
            for (const StepForces& forces : nodalForcesOf(context, ref, request.nodeIds))
            {
                if (sink.covers(forces.set, forces.subcase, forces.designCycle, forces.step))
                    continue;
                if (sink.add(resultantOf(forces, request.point, positions)))
                    ++outcome.rows;
            }
        if (outcome.rows == 0)
            outcome.reason = "the table has no real values for the steps asked for";
        return outcome;
    }
};

// Applied loads, second way: the load cards of the model (the `loadResultant` query), for the static subcases that no
// applied-load table covers.
class LoadCardsProvider final : public ResultantProvider
{
public:
    [[nodiscard]] std::string name() const override { return "load cards of the model"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ResultantRequest& request,
                                  ResultantSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the load cards are in the model)"};
        if (request.nodeIds)
            return {0, "the load cards give the load of the model, not of the grid points asked for"};
        Outcome outcome;
        std::string skipped;
        for (std::size_t set = 0; set < context.sets().size(); ++set)
        {
            // the static subcases the results hold, which no earlier way covered
            std::vector<std::int64_t> wanted;
            std::set<std::int32_t> seen;
            for (const std::shared_ptr<results::ResultTable>& table : context.sets()[set].tables->all())
            {
                const results::ResultTableMeta& meta = table->meta();
                if (context.selection().subcases &&
                    std::find(context.selection().subcases->begin(), context.selection().subcases->end(),
                              meta.subcase) == context.selection().subcases->end())
                    continue;
                if (meta.analysisCode != 1)
                {
                    skipped =
                        "the load cards give the amplitude of a load, not its value at a time or a frequency "
                        "(dynamic subcases are left to the other ways)";
                    continue;
                }
                if (seen.insert(meta.subcase).second && !sink.covers(set, meta.subcase, meta.designCycle, std::nan("")))
                    wanted.push_back(meta.subcase);
            }
            if (wanted.empty())
                continue;
            QueryArguments arguments;
            arguments.set("subcases", wanted);
            arguments.set("point", std::vector<double>{request.point[0], request.point[1], request.point[2]});
            arguments.set("frame", std::string("basic"));
            const QueryRun run{context.modelPointer(), arguments, context.exec(), nullptr, {}};
            const std::shared_ptr<const QueryTable> table = NamedQueryRegistry::instance().run("loadResultant", run);
            const auto subcases = asInt32(table->column("subcase"));
            // what the query says of its sources: the load cards it does not sum (RFORCE and PLOAD1) are named there
            const auto sources = asTexts(table->column("source"));
            std::array<std::vector<double>, 6> values;
            const char* const names[6] = {"fx", "fy", "fz", "mx", "my", "mz"};
            for (std::size_t component = 0; component < 6; ++component)
                values[component] = asDoubles(table->column(names[component]));
            for (std::size_t row = 0; row < subcases.size(); ++row)
            {
                ResultantRow resultant;
                resultant.set = set;
                resultant.analysis = context.sets()[set].analysis;
                resultant.subcase = subcases[row];
                resultant.precision = Precision::Double;
                for (std::size_t component = 0; component < 6; ++component)
                    resultant.value[component] = values[component][row];
                resultant.count = 0;
                resultant.note =
                    row < sources.size() && !sources[row].empty() ? sources[row] : std::string("load cards");
                if (sink.add(std::move(resultant)))
                    ++outcome.rows;
            }
        }
        if (outcome.rows == 0)
            outcome.reason = !skipped.empty() ? skipped : "every static subcase was already given, or none is static";
        return outcome;
    }
};

} // namespace

const ResultantChain& reactionChain()
{
    static const ResultantChain CHAIN = []
    {
        ResultantChain chain("reactions");
        chain.add(std::make_unique<SinglePointConstrForceTableProvider>());
        chain.add(std::make_unique<NodePointForceBalanceProvider>());
        return chain;
    }();
    return CHAIN;
}

const ResultantChain& appliedLoadChain()
{
    static const ResultantChain CHAIN = []
    {
        ResultantChain chain("applied load resultant");
        chain.add(std::make_unique<AppliedLoadTableProvider>());
        chain.add(std::make_unique<LoadCardsProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
