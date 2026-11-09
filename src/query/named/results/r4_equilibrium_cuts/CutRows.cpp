#include "CutRows.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/providers/quantities/CutConnections.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <set>

namespace oor::query
{

namespace
{

constexpr std::array<const char*, 6> NAMES = {"fx", "fy", "fz", "mx", "my", "mz"};

bool isStepped(results::StepKind kind) noexcept
{
    return kind == results::StepKind::Time || kind == results::StepKind::Frequency || kind == results::StepKind::Mode ||
           kind == results::StepKind::LoadFactor;
}

std::array<double, 6> turned(const base::Mat3& transposedAxes, const std::array<double, 6>& value)
{
    const base::Vec3 force = transposedAxes * base::Vec3{value[0], value[1], value[2]};
    const base::Vec3 moment = transposedAxes * base::Vec3{value[3], value[4], value[5]};
    return {force.x, force.y, force.z, moment.x, moment.y, moment.z};
}

// The order of the rows of a table: design cycle, result set, the subcases of the file before the combinations (in the
// order they were given), subcase, step.
std::vector<std::size_t> rowOrder(const std::vector<ResultantRow>& rows)
{
    std::map<std::string, std::size_t> combinationOrder;
    for (const ResultantRow& row : rows)
        if (!row.combination.empty())
            combinationOrder.emplace(row.combination, combinationOrder.size() + 1);
    std::vector<std::size_t> order(rows.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    const auto combinationRank = [&](const ResultantRow& row)
    { return row.combination.empty() ? std::size_t{0} : combinationOrder.at(row.combination); };
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right)
                     {
                         const ResultantRow& a = rows[left];
                         const ResultantRow& b = rows[right];
                         if (a.designCycle != b.designCycle)
                             return a.designCycle < b.designCycle;
                         if (a.set != b.set)
                             return a.set < b.set;
                         if (combinationRank(a) != combinationRank(b))
                             return combinationRank(a) < combinationRank(b);
                         if (a.subcase != b.subcase)
                             return a.subcase < b.subcase;
                         return a.step < b.step || (std::isnan(b.step) && !std::isnan(a.step));
                     });
    return order;
}

// True when two rows can share a block: one result set, design cycle, step kind, precision and value kind.
bool sameBlock(const ResultantRow& a, const ResultantRow& b)
{
    return a.set == b.set && a.designCycle == b.designCycle && a.stepKind == b.stepKind && a.precision == b.precision &&
           a.isComplex == b.isComplex;
}

} // namespace

std::vector<ResultantRow> resolveCut(const ResultQueryContext& context, const ResultantRequest& request,
                                     std::vector<ProviderInfo>& providers, std::vector<ProviderAttempt>& attempts)
{
    ResultantSet set;
    attempts = cutChain().resolve(context, request, set);
    providers = set.providers();
    return std::move(set.mutableRows());
}

void addCombinations(std::vector<ResultantRow>& rows, const std::vector<NamedCombination>& combinations)
{
    if (combinations.empty())
        return;
    std::vector<ResultantRow> added;
    // the result sets and design cycles the rows come from
    std::set<std::pair<std::size_t, std::int32_t>> origins;
    for (const ResultantRow& row : rows)
        origins.emplace(row.set, row.designCycle);
    for (const NamedCombination& combination : combinations)
    {
        bool anywhere = false;
        for (const auto& [set, cycle] : origins)
        {
            std::vector<std::pair<const ResultantRow*, double>> terms;
            std::vector<std::int32_t> missing;
            for (const auto& [subcase, factor] : combination.factors)
            {
                const ResultantRow* found = nullptr;
                bool stepped = false;
                for (const ResultantRow& row : rows)
                {
                    if (row.set != set || row.designCycle != cycle || row.subcase != subcase ||
                        !row.combination.empty())
                        continue;
                    if (isStepped(row.stepKind))
                        stepped = true;
                    else
                        found = &row;
                }
                if (stepped)
                    throw QueryError("the combination '" + combination.name + "' takes subcase " +
                                     std::to_string(subcase) +
                                     ", which is not static: a combination adds the results of static subcases");
                if (found == nullptr)
                    missing.push_back(subcase);
                else
                    terms.emplace_back(found, factor);
            }
            if (terms.empty())
                continue;
            if (!missing.empty())
                throw QueryError("the combination '" + combination.name + "' takes subcase " +
                                 std::to_string(missing.front()) + ", which the results of '" +
                                 terms.front().first->analysis +
                                 "' do not hold (or the call left out: give it in `subcases`)");
            anywhere = true;
            ResultantRow sum = *terms.front().first;
            sum.combination = combination.name;
            sum.subcase = 0;
            sum.value.fill(0.0);
            sum.imaginary.fill(0.0);
            sum.nodes.clear();
            sum.uncountedCards.clear();
            sum.uncountedIds.clear();
            sum.complete = true;
            sum.count = 0;
            sum.elementCount = 0;
            std::set<std::pair<std::string, std::int32_t>> uncounted;
            std::map<std::pair<std::int32_t, std::string>, std::size_t> nodeIndex;
            for (const auto& [row, factor] : terms)
            {
                for (std::size_t component = 0; component < 6; ++component)
                {
                    sum.value[component] += factor * row->value[component];
                    sum.imaginary[component] += factor * row->imaginary[component];
                }
                for (const CutNodeForce& node : row->nodes)
                {
                    const auto key = std::make_pair(node.node, node.contribution);
                    auto found = nodeIndex.find(key);
                    if (found == nodeIndex.end())
                    {
                        found = nodeIndex.emplace(key, sum.nodes.size()).first;
                        sum.nodes.push_back(CutNodeForce{node.node, node.contribution, {}, {}});
                    }
                    CutNodeForce& target = sum.nodes[found->second];
                    for (std::size_t component = 0; component < 6; ++component)
                    {
                        target.real[component] += factor * node.real[component];
                        target.imaginary[component] += factor * node.imaginary[component];
                    }
                }
                sum.complete = sum.complete && row->complete;
                for (std::size_t index = 0; index < row->uncountedIds.size(); ++index)
                    uncounted.emplace(row->uncountedCards[index], row->uncountedIds[index]);
                sum.count = std::max(sum.count, row->count);
                sum.elementCount = std::max(sum.elementCount, row->elementCount);
                sum.provider = std::max(sum.provider, row->provider);
                sum.approximate = sum.approximate || row->approximate;
                if (row->precision == Precision::Double)
                    sum.precision = Precision::Double;
            }
            for (const auto& [card, id] : uncounted)
            {
                sum.uncountedCards.push_back(card);
                sum.uncountedIds.push_back(id);
            }
            added.push_back(std::move(sum));
        }
        if (!anywhere)
            throw QueryError("the combination '" + combination.name +
                             "' takes subcases that no result set of the call holds");
    }
    rows.insert(rows.end(), std::make_move_iterator(added.begin()), std::make_move_iterator(added.end()));
}

base::Mat3 cutAxes(const std::array<double, 3>& normal, const std::optional<std::array<double, 3>>& xAxis)
{
    const base::Vec3 z = base::normalized(base::Vec3{normal[0], normal[1], normal[2]});
    base::Vec3 reference{1.0, 0.0, 0.0};
    if (xAxis)
        reference = base::Vec3{(*xAxis)[0], (*xAxis)[1], (*xAxis)[2]};
    else if (std::fabs(base::dot(reference, z)) > 0.9)
        reference = base::Vec3{0.0, 1.0, 0.0};
    const base::Vec3 inPlane = reference - base::dot(reference, z) * z;
    if (!(base::norm(inPlane) > 1.0e-9 * base::norm(reference)))
        throw QueryError("xAxis is along the normal of the cut: it gives no x axis in the plane of the cut");
    const base::Vec3 x = base::normalized(inPlane);
    return base::Mat3::fromColumns(x, base::cross(z, x), z);
}

void turnRows(std::vector<ResultantRow>& rows, const base::Mat3& axes)
{
    const base::Mat3 transposed = axes.transposed();
    for (ResultantRow& row : rows)
    {
        row.value = turned(transposed, row.value);
        row.imaginary = turned(transposed, row.imaginary);
        for (CutNodeForce& node : row.nodes)
        {
            node.real = turned(transposed, node.real);
            node.imaginary = turned(transposed, node.imaginary);
        }
    }
}

void turnRowsToSystem(const ResultQueryContext& context, std::vector<ResultantRow>& rows, std::int64_t system)
{
    if (system == 0)
        return;
    for (ResultantRow& row : rows)
    {
        row.value = resultantInSystem(context, row.set, row.value, system);
        row.imaginary = resultantInSystem(context, row.set, row.imaginary, system);
        for (CutNodeForce& node : row.nodes)
        {
            node.real = resultantInSystem(context, row.set, node.real, system);
            node.imaginary = resultantInSystem(context, row.set, node.imaginary, system);
        }
    }
}

bool hasLoadPath(const ResultQueryContext& context, const CutDefinition& cut,
                 std::unique_ptr<CutConnections>* connections)
{
    if (!cut.nodes.empty())
        return true;
    const model::Model& model = context.requireModel("find the connections across the cut");
    if (connections == nullptr)
        return !CutConnections(model, std::nullopt, context.exec().pool()).crossing(cut).empty();
    if (!*connections)
        *connections = std::make_unique<CutConnections>(model, std::nullopt, context.exec().pool());
    return !(*connections)->crossing(cut).empty();
}

std::optional<std::array<double, 3>> xAxisOf(const QueryArguments& arguments)
{
    if (!arguments.has("xAxis"))
        return std::nullopt;
    const std::vector<double> values = arguments.reals("xAxis");
    if (values.size() != 3)
        throw QueryError("xAxis is a vector of three numbers in the basic system, not " +
                         std::to_string(values.size()) + " numbers");
    return std::array<double, 3>{values[0], values[1], values[2]};
}

std::vector<std::int16_t> registerCutProviders(const std::vector<ProviderInfo>& providers, RowAccumulator& accumulator)
{
    std::vector<std::int16_t> mapped;
    for (const ProviderInfo& info : providers)
        mapped.push_back(accumulator.addProvider(info.name, info.approximate));
    return mapped;
}

void addCutRows(const std::vector<ResultantRow>& rows, const std::vector<ProviderInfo>& providers,
                const std::array<double, 3>& point, const CutColumns& columns, RowAccumulator& accumulator,
                const std::vector<std::int16_t>& providerIndex)
{
    for (RowBlock& block : cutBlocks(rows, providers, point, columns, providerIndex))
        accumulator.add(std::move(block));
}

std::vector<RowBlock> cutBlocks(const std::vector<ResultantRow>& rows, const std::vector<ProviderInfo>& providers,
                                const std::array<double, 3>& point, const CutColumns& columns,
                                const std::vector<std::int16_t>& providerIndex)
{
    std::vector<RowBlock> blocks;
    const std::vector<std::size_t> order = rowOrder(rows);
    std::size_t begin = 0;
    while (begin < order.size())
    {
        const ResultantRow& first = rows[order[begin]];
        std::size_t end = begin + 1;
        while (end < order.size() && sameBlock(first, rows[order[end]]))
            ++end;
        const std::size_t count = end - begin;
        RowBlock block(count);
        std::vector<std::string> combination;
        std::vector<std::uint8_t> combinationValid;
        std::vector<std::int32_t> subcase;
        std::vector<std::uint8_t> subcaseValid;
        std::array<std::vector<double>, 6> real;
        std::array<std::vector<double>, 6> imaginary;
        std::vector<std::int32_t> nodes;
        std::vector<std::int32_t> elements;
        std::vector<std::uint8_t> complete;
        std::vector<std::vector<std::string>> cards;
        std::vector<std::vector<std::int32_t>> ids;
        std::vector<double> steps;
        std::vector<double> eigenvalues;
        std::vector<std::int16_t> provider;
        std::vector<std::string> source;
        for (std::size_t position = begin; position < end; ++position)
        {
            const ResultantRow& row = rows[order[position]];
            combination.push_back(row.combination);
            combinationValid.push_back(row.combination.empty() ? 0 : 1);
            subcase.push_back(row.subcase);
            subcaseValid.push_back(row.combination.empty() ? 1 : 0);
            for (std::size_t component = 0; component < 6; ++component)
            {
                real[component].push_back(row.value[component]);
                imaginary[component].push_back(row.imaginary[component]);
            }
            nodes.push_back(static_cast<std::int32_t>(row.count));
            elements.push_back(static_cast<std::int32_t>(row.elementCount));
            complete.push_back(row.complete ? 1 : 0);
            cards.push_back(row.uncountedCards);
            ids.push_back(row.uncountedIds);
            steps.push_back(row.step);
            eigenvalues.push_back(row.eigenvalue);
            provider.push_back(providerIndex[static_cast<std::size_t>(row.provider)]);
            const ProviderInfo& info = providers[static_cast<std::size_t>(row.provider)];
            source.push_back(sourceText(row.note.empty() ? info.name : row.note, info.approximate));
        }
        if (columns.withCutName)
        {
            block.texts("cut", std::vector<std::string>(count, columns.cutName));
            block.reals("station", std::vector<double>(count, columns.station));
        }
        block.texts("combination", std::move(combination), std::move(combinationValid));
        block.ints("subcase", std::move(subcase), std::move(subcaseValid));
        for (std::size_t component = 0; component < 6; ++component)
        {
            if (first.isComplex)
                block.complex(NAMES[component], std::move(real[component]), std::move(imaginary[component]));
            else
                block.reals(NAMES[component], std::move(real[component]));
        }
        block.reals("pointX", std::vector<double>(count, point[0]));
        block.reals("pointY", std::vector<double>(count, point[1]));
        block.reals("pointZ", std::vector<double>(count, point[2]));
        block.ints(columns.countColumn, std::move(nodes));
        if (!columns.elementCountColumn.empty())
            block.ints(columns.elementCountColumn, std::move(elements));
        block.flags("isComplete", std::move(complete));
        block.textLists("uncountedCards", std::move(cards));
        block.intLists("uncountedIds", std::move(ids));
        block.texts("source", std::move(source));
        block.providers(std::move(provider));
        if (isStepped(first.stepKind))
        {
            const bool anyEigen =
                std::any_of(eigenvalues.begin(), eigenvalues.end(), [](double value) { return !std::isnan(value); });
            block.steps(first.stepKind, std::move(steps), anyEigen ? std::move(eigenvalues) : std::vector<double>());
        }
        block.origin(first.set, first.analysis, first.designCycle);
        block.precision(first.precision);
        block.storedAsMagnitudePhase(first.magnitudePhase);
        blocks.push_back(std::move(block));
        begin = end;
    }
    return blocks;
}

void addCutNodeRows(const std::vector<ResultantRow>& rows, RowAccumulator& accumulator, std::int16_t provider)
{
    const std::vector<std::size_t> order = rowOrder(rows);
    std::size_t begin = 0;
    while (begin < order.size())
    {
        const ResultantRow& first = rows[order[begin]];
        std::size_t end = begin + 1;
        while (end < order.size() && sameBlock(first, rows[order[end]]))
            ++end;
        std::size_t count = 0;
        for (std::size_t position = begin; position < end; ++position)
            count += rows[order[position]].nodes.size();
        if (count > 0)
        {
            RowBlock block(count);
            std::vector<std::string> combination;
            std::vector<std::uint8_t> combinationValid;
            std::vector<std::int32_t> subcase;
            std::vector<std::uint8_t> subcaseValid;
            std::vector<std::int32_t> nid;
            std::vector<std::string> contribution;
            std::array<std::vector<double>, 6> real;
            std::array<std::vector<double>, 6> imaginary;
            std::vector<double> steps;
            std::vector<double> eigenvalues;
            for (std::size_t position = begin; position < end; ++position)
            {
                const ResultantRow& row = rows[order[position]];
                for (const CutNodeForce& node : row.nodes)
                {
                    combination.push_back(row.combination);
                    combinationValid.push_back(row.combination.empty() ? 0 : 1);
                    subcase.push_back(row.subcase);
                    subcaseValid.push_back(row.combination.empty() ? 1 : 0);
                    nid.push_back(node.node);
                    contribution.push_back(node.contribution);
                    for (std::size_t component = 0; component < 6; ++component)
                    {
                        real[component].push_back(node.real[component]);
                        imaginary[component].push_back(node.imaginary[component]);
                    }
                    steps.push_back(row.step);
                    eigenvalues.push_back(row.eigenvalue);
                }
            }
            block.texts("combination", std::move(combination), std::move(combinationValid));
            block.ints("subcase", std::move(subcase), std::move(subcaseValid));
            block.ints("nodeId", std::move(nid));
            for (std::size_t component = 0; component < 6; ++component)
            {
                if (first.isComplex)
                    block.complex(NAMES[component], std::move(real[component]), std::move(imaginary[component]));
                else
                    block.reals(NAMES[component], std::move(real[component]));
            }
            block.texts("contribution", std::move(contribution));
            if (isStepped(first.stepKind))
            {
                const bool anyEigen = std::any_of(eigenvalues.begin(), eigenvalues.end(),
                                                  [](double value) { return !std::isnan(value); });
                block.steps(first.stepKind, std::move(steps),
                            anyEigen ? std::move(eigenvalues) : std::vector<double>());
            }
            block.origin(first.set, first.analysis, first.designCycle);
            block.precision(first.precision);
            block.storedAsMagnitudePhase(first.magnitudePhase);
            block.provider(provider);
            accumulator.add(std::move(block));
        }
        begin = end;
    }
}

} // namespace oor::query
