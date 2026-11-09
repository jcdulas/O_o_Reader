#include "oor/query/named/results/ResultQuery.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/EnvelopeOperator.hpp"
#include "oor/query/plan/Query.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <span>
#include <tuple>

namespace oor::query
{

namespace
{

// The `envelope` option of the result queries: the critical row of each group of rows that differ only in the
// dimensions the envelope is over.
std::shared_ptr<const QueryTable> envelopeOf(const ResultQueryContext& context, const QueryInfo& info,
                                             const std::shared_ptr<const QueryTable>& table)
{
    const ResultSelection& selection = context.selection();
    EnvelopeSpec spec;
    spec.value = selection.envelopeValue.empty() ? info.envelopeValue : selection.envelopeValue;
    spec.by = envelopeByFromName(selection.envelope);
    spec.over.clear();
    for (const std::string& word : selection.envelopeOver)
    {
        if (word == "analyses")
        {
            if (!table->has("analysis"))
                throw QueryError("an envelope over the analyses needs more than one result set: " +
                                 std::to_string(context.attachedSetCount()) + " is attached");
            spec.over.push_back("analysis");
        }
        else if (word == "subcases")
        {
            if (table->has("subcase"))
                spec.over.push_back("subcase");
        }
        else if (word == "designCycles")
        {
            if (table->has("designCycle"))
                spec.over.push_back("designCycle");
        }
        else
        {
            for (const char* step : {"time", "freq", "mode", "loadStep", "eigenvalue"})
                if (table->has(step))
                    spec.over.push_back(step);
        }
    }
    if (spec.over.empty())
        return table;
    if (!info.envelopeKeys.empty())
    {
        for (const std::string& name : info.envelopeKeys)
            if (table->has(name) && std::find(spec.over.begin(), spec.over.end(), name) == spec.over.end())
                spec.keys.push_back(name);
        for (const char* step : {"time", "freq", "mode", "loadStep"})
            if (table->has(step) && std::find(spec.over.begin(), spec.over.end(), step) == spec.over.end())
                spec.keys.push_back(step);
    }
    return envelopeTable(*table, spec, context.exec());
}

} // namespace

std::shared_ptr<const QueryTable> ResultQuery::run(const QueryRun& run) const
{
    const ResultQueryContext context(run, info());
    if (!context.selection().envelope.empty() && info().envelopeValue.empty())
        throw QueryError("the query '" + info().name +
                         "' takes no envelope (envelopeBy= is for the queries that give a quantity per item: "
                         "displacementsTable, stressTable, maxDisplacement, ...)");
    std::shared_ptr<const QueryTable> table = runWith(context, run);
    if (context.selection().envelope.empty())
        return table;
    return envelopeOf(context, info(), table);
}

void sortTables(std::vector<TableRef>& tables)
{
    std::stable_sort(tables.begin(), tables.end(),
                     [](const TableRef& left, const TableRef& right)
                     {
                         const results::ResultTableMeta& a = left.table->meta();
                         const results::ResultTableMeta& b = right.table->meta();
                         return std::tie(a.designCycle, left.set, a.subcase) <
                                std::tie(b.designCycle, right.set, b.subcase);
                     });
}

std::vector<std::string> leadingKeys()
{
    return {"designCycle", "analysis", "subcase", "time", "freq", "mode", "loadStep"};
}

namespace
{

// True when the rows of `table` are in the order of the columns `keys` already (each an integer or real column with no
// null and no NaN, the rows non-decreasing lexicographically): the stable sort would give them as they are.
bool inKeyOrder(const QueryTable& table, const std::vector<std::string>& keys)
{
    std::vector<std::span<const std::int32_t>> ints;
    std::vector<std::span<const std::int64_t>> longs;
    std::vector<std::span<const double>> reals;
    std::vector<std::pair<int, std::size_t>> order; // the kind (0 int32, 1 int64, 2 real) and the place of each key
    for (const std::string& name : keys)
    {
        const QueryColumn& column = table.column(name);
        if (column.hasNulls())
            return false;
        switch (column.type())
        {
        case ColumnType::Int32:
            order.emplace_back(0, ints.size());
            ints.push_back(column.as<std::int32_t>());
            break;
        case ColumnType::Int64:
            order.emplace_back(1, longs.size());
            longs.push_back(column.as<std::int64_t>());
            break;
        case ColumnType::Float64:
            order.emplace_back(2, reals.size());
            reals.push_back(column.as<double>());
            for (const double value : reals.back())
                if (std::isnan(value))
                    return false;
            break;
        default:
            return false;
        }
    }
    const auto rows = static_cast<std::size_t>(table.rowCount());
    for (std::size_t row = 1; row < rows; ++row)
        for (const auto& [kind, at] : order)
        {
            const auto compare = [&](const auto& values) -> int
            { return values[row - 1] < values[row] ? -1 : (values[row] < values[row - 1] ? 1 : 0); };
            const int sign = kind == 0 ? compare(ints[at]) : kind == 1 ? compare(longs[at]) : compare(reals[at]);
            if (sign < 0)
                break; // this row comes after the one before: in order
            if (sign > 0)
                return false;
        }
    return true;
}

} // namespace

std::shared_ptr<const QueryTable> sortedResult(const std::shared_ptr<const QueryTable>& table,
                                               ExecutionContext& context, const std::vector<std::string>& keys)
{
    std::vector<SortKey> sortKeys;
    std::vector<std::string> present;
    for (const std::string& name : keys)
        if (table->has(name))
        {
            SortKey key;
            key.column = name;
            sortKeys.push_back(std::move(key));
            present.push_back(name);
        }
    if (sortKeys.empty() || table->rowCount() < 2)
        return table;
    // PERFORMANCE (COMPOSITE-API-AUDIT P2): rows in the order of the keys already need no sort (a table of millions
    // of plies is made in that order)
    if (inKeyOrder(*table, present))
        return table;
    Query query(std::make_shared<const TableSource>(table));
    query = query.sortBy(std::move(sortKeys));
    const std::shared_ptr<const QueryTable> sorted = query.run(context);
    if (table->sources().empty())
        return sorted;
    auto carried = std::make_shared<QueryTable>(*sorted);
    carried->setSources(table->sources());
    return carried;
}

Precision precisionOf(const results::ResultData& data) noexcept
{
    return data.type == results::ValueType::Float32 || data.type == results::ValueType::Complex64 ? Precision::Single
                                                                                                  : Precision::Double;
}

void addStepsToBlock(RowBlock& block, const results::ResultTableMeta& meta, const results::ResultData& data,
                     const std::vector<std::size_t>& stepOf)
{
    switch (meta.stepKind)
    {
    case results::StepKind::Time:
    case results::StepKind::Frequency:
    case results::StepKind::Mode:
    case results::StepKind::LoadFactor:
        break;
    default:
        return;
    }
    std::vector<double> values(stepOf.size());
    std::vector<double> eigenvalues;
    if (meta.stepKind == results::StepKind::Mode && !data.eigenvalues.empty())
        eigenvalues.resize(stepOf.size());
    for (std::size_t row = 0; row < stepOf.size(); ++row)
    {
        values[row] = stepOf[row] < data.steps.size() ? data.steps[stepOf[row]] : std::nan("");
        if (!eigenvalues.empty())
            eigenvalues[row] = data.eigenvalues[stepOf[row]];
    }
    block.steps(meta.stepKind, std::move(values), std::move(eigenvalues));
}

std::array<double, 3> ResultNodePositions::position(std::size_t set, std::int32_t resultId)
{
    const std::array<double, 3> none = {std::nan(""), std::nan(""), std::nan("")};
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr)
        return none;
    const std::int32_t modelId = m_context.modelNodeId(set, resultId);
    if (modelId <= 0)
        return none;
    auto found = m_positions.find(model);
    if (found == m_positions.end())
        found = m_positions.emplace(model, std::make_unique<NodePositions>(*model, m_context.exec().pool())).first;
    if (!found->second->has(modelId))
        return none;
    const base::Vec3 point = found->second->position(modelId);
    return {point.x, point.y, point.z};
}

PointRows pointRows(const ResultQueryContext& context, const TableRef& ref, const std::vector<std::string>& components,
                    const std::optional<std::vector<std::int64_t>>& keep, bool withFrame)
{
    const results::ResultTableMeta& meta = ref.table->meta();
    const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, std::nullopt, keep);
    if (!decoded)
        return PointRows{RowBlock(0), nullptr, {}, {}};
    const TableValues values(*decoded);
    std::set<std::int64_t> wanted;
    if (keep)
        wanted.insert(keep->begin(), keep->end());
    std::vector<std::size_t> rowsKept;
    for (std::size_t row = 0; row < values.rows(); ++row)
        if (!keep || wanted.contains(decoded->ids[row]))
            rowsKept.push_back(row);
    const std::size_t total = values.steps() * rowsKept.size();
    PointRows result{RowBlock(total), decoded, {}, {}};
    result.rows.reserve(total);
    result.stepOf.reserve(total);
    std::vector<std::int32_t> subcase(total, meta.subcase);
    std::vector<std::int32_t> nid;
    nid.reserve(total);
    for (std::size_t step = 0; step < values.steps(); ++step)
        for (const std::size_t row : rowsKept)
        {
            result.rows.push_back(row);
            result.stepOf.push_back(step);
            nid.push_back(decoded->ids[row]);
        }
    result.block.ints("subcase", std::move(subcase));
    result.block.ints("nodeId", std::move(nid));
    for (const std::string& name : components)
    {
        const int component = values.component(name);
        std::vector<double> real(total, std::nan(""));
        std::vector<double> imaginary;
        if (values.isComplex())
            imaginary.assign(total, std::nan(""));
        if (component >= 0)
        {
            for (std::size_t index = 0; index < total; ++index)
            {
                real[index] =
                    values.real(result.stepOf[index], result.rows[index], static_cast<std::size_t>(component));
                if (values.isComplex())
                    imaginary[index] =
                        values.imag(result.stepOf[index], result.rows[index], static_cast<std::size_t>(component));
            }
        }
        if (values.isComplex())
            result.block.complex(name, std::move(real), std::move(imaginary));
        else
            result.block.reals(name, std::move(real));
    }
    addStepsToBlock(result.block, meta, *decoded, result.stepOf);
    result.block.origin(ref.set, context.sets()[ref.set].analysis, meta.designCycle);
    result.block.precision(precisionOf(*decoded));
    result.block.storedAsMagnitudePhase(meta.storedFormat == results::ResultFormat::MagnitudePhase);
    result.block.validation(std::string(results::resultStatusName(meta.status)));
    if (withFrame)
        result.block.texts("frame", std::vector<std::string>(total, context.frameNameOf(ref)));
    return result;
}

} // namespace oor::query
