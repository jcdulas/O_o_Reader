#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace oor::query
{

namespace
{

struct Line
{
    std::uint32_t element; // position in the list of 1D elements
    std::int32_t a;
    std::int32_t b;
    std::int32_t weldedA; // the welded grid points the ends stand on (WeldedNodes): what the chains follow
    std::int32_t weldedB;
};

struct Step
{
    std::uint32_t line;
    std::int32_t from; // the grid points of the element, in the direction of the chain
    std::int32_t to;
};

struct Chain
{
    std::vector<Step> steps;
    bool closed = false;
};

// elementChains: structural 1D elements followed end to end into ordered lines.
class ElementChainsQuery final : public QueryBase
{
public:
    ElementChainsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<std::uint16_t> types;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        if (typeNames.empty())
        {
            for (const std::uint16_t type : typesOfCategory(model::CardCategory::Element))
            {
                if (analysis::roleOfType(type) == analysis::ElementRole::Line)
                    types.push_back(type);
            }
        }
        else
        {
            for (const std::uint16_t type : typesNamed(typeNames, model::CardCategory::Element, "types"))
            {
                if (analysis::roleOfType(type) == analysis::ElementRole::Line)
                    types.push_back(type);
            }
        }
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        const NodeLists nodes = nodesOfCards(model, list, run.context);
        // Grid points welded by rigid elements of zero length are one point: a chain goes on across the weld.
        const std::shared_ptr<const analysis::WeldedNodes> welds =
            analysis::ComponentCache::of(model).welds(model, run.context.pool());
        std::vector<Line> lines;
        for (std::size_t index = 0; index < list.size(); ++index)
        {
            const auto ends = nodes.of(index);
            if (ends.size() < 2)
                continue;
            const std::int32_t weldedA = welds->representative(ends[0]);
            const std::int32_t weldedB = welds->representative(ends[1]);
            if (weldedA != weldedB)
                lines.push_back(Line{static_cast<std::uint32_t>(index), ends[0], ends[1], weldedA, weldedB});
        }
        if (lines.empty())
            return emptyTable(info().defaultColumns());

        std::vector<std::pair<std::int32_t, std::uint32_t>> incident;
        for (std::size_t line = 0; line < lines.size(); ++line)
        {
            incident.emplace_back(lines[line].weldedA, static_cast<std::uint32_t>(line));
            incident.emplace_back(lines[line].weldedB, static_cast<std::uint32_t>(line));
        }
        std::sort(incident.begin(), incident.end());
        const auto edgesAt = [&](std::int32_t node)
        {
            const auto first =
                std::lower_bound(incident.begin(), incident.end(), std::make_pair(node, std::uint32_t{0}));
            auto last = first;
            while (last != incident.end() && last->first == node)
                ++last;
            return std::pair{first, last};
        };
        std::vector<std::uint8_t> visited(lines.size(), 0);
        std::vector<Chain> chains;
        // The next unvisited element at a node, the smallest first (the lines are in order of element ID).
        const auto nextAt = [&](std::int32_t node) -> std::int64_t
        {
            const auto [first, last] = edgesAt(node);
            for (auto at = first; at != last; ++at)
            {
                if (visited[at->second] == 0)
                    return at->second;
            }
            return -1;
        };
        const auto walk = [&](std::int32_t start, std::uint32_t first)
        {
            Chain chain;
            std::int32_t current = start;
            std::int64_t line = first;
            while (line >= 0)
            {
                visited[static_cast<std::size_t>(line)] = 1;
                const Line& item = lines[static_cast<std::size_t>(line)];
                const bool forward = item.weldedA == current;
                chain.steps.push_back(
                    Step{static_cast<std::uint32_t>(line), forward ? item.a : item.b, forward ? item.b : item.a});
                current = forward ? item.weldedB : item.weldedA;
                // A chain goes on through a node where exactly two elements meet; it stops at a free end or a junction.
                const auto [begin, end] = edgesAt(current);
                line = end - begin == 2 ? nextAt(current) : -1;
            }
            chain.closed = chain.steps.size() > 1 && current == start;
            chains.push_back(std::move(chain));
        };
        std::vector<std::int32_t> nodeList;
        for (const auto& entry : incident)
        {
            if (nodeList.empty() || nodeList.back() != entry.first)
                nodeList.push_back(entry.first);
        }
        for (const std::int32_t node : nodeList)
        {
            const auto [first, last] = edgesAt(node);
            if (last - first == 2)
                continue;
            for (std::int64_t line = nextAt(node); line >= 0; line = nextAt(node))
                walk(node, static_cast<std::uint32_t>(line));
        }
        for (std::size_t line = 0; line < lines.size(); ++line)
        {
            if (visited[line] == 0)
                walk(lines[line].weldedA, static_cast<std::uint32_t>(line));
        }
        // Chains are numbered by the smallest element ID they hold.
        std::vector<std::int32_t> smallest(chains.size(), std::numeric_limits<std::int32_t>::max());
        for (std::size_t index = 0; index < chains.size(); ++index)
        {
            for (const Step& step : chains[index].steps)
                smallest[index] = std::min(smallest[index], list.ids[lines[step.line].element]);
        }
        std::vector<std::size_t> order(chains.size());
        std::iota(order.begin(), order.end(), std::size_t{0});
        std::sort(order.begin(), order.end(), [&](std::size_t x, std::size_t y) { return smallest[x] < smallest[y]; });

        ListColumns columns(run.model, list, run.context);
        const std::vector<std::int32_t> pids = asInt32(columns.get("propId"));
        const std::vector<double> lengths = columns.reals("length");
        std::vector<std::int64_t> chainId;
        std::vector<std::int32_t> position;
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        std::vector<std::int32_t> pid;
        std::vector<std::uint8_t> pidValid;
        std::vector<std::int32_t> start;
        std::vector<std::int32_t> end;
        std::vector<double> length;
        std::vector<double> cumulative;
        std::vector<std::uint8_t> closed;
        for (std::size_t number = 0; number < order.size(); ++number)
        {
            const Chain& chain = chains[order[number]];
            double running = 0.0;
            for (std::size_t at = 0; at < chain.steps.size(); ++at)
            {
                const Step& step = chain.steps[at];
                const std::uint32_t element = lines[step.line].element;
                chainId.push_back(static_cast<std::int64_t>(number + 1));
                position.push_back(static_cast<std::int32_t>(at + 1));
                eid.push_back(list.ids[element]);
                type.emplace_back(typeName(list.types[element]));
                pid.push_back(pids[element]);
                pidValid.push_back(pids[element] != 0 ? 1 : 0);
                start.push_back(step.from);
                end.push_back(step.to);
                length.push_back(lengths[element]);
                running += std::isfinite(lengths[element]) ? lengths[element] : 0.0;
                cumulative.push_back(running);
                closed.push_back(chain.closed ? 1 : 0);
            }
        }
        TableBuilder builder;
        builder.int64("chainId", std::move(chainId));
        builder.int32("position", std::move(position));
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.int32("propId", std::move(pid), std::move(pidValid));
        builder.int32("nStart", std::move(start));
        builder.int32("nEnd", std::move(end));
        builder.real("length", std::move(length));
        builder.real("cumulativeLength", std::move(cumulative));
        builder.flag("chainIsClosed", closed);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementChains";
        info.family = "T4";
        info.description =
            "The structural 1D elements (rods, tubes, bars, beams, bends) followed end to end into ordered lines, such "
            "as "
            "stiffeners: a chain stops at a node where three 1D elements or more meet, or at a free end. chainId and "
            "position are from 1; nStart -> nEnd follows the walk, which starts at the end node with the smaller ID "
            "(a closed ring starts at the first end of its smallest element); cumulativeLength is the length from the "
            "start of the chain to the end of the element. Chains are numbered by the smallest element ID they hold. "
            "Grid points that rigid elements of zero length weld in all six components (one element or several) are "
            "one point: a chain goes on across the weld, and nStart and nEnd are the grid points of each element.";
        info.parameters = {parameter("types", ParameterKind::TextList, "None", "keep these card types"),
                           parameter("elementIds", ParameterKind::IdList, "None", "only these elements are chained")};
        info.defaultSort = "chainId, position";
        ColumnListBuilder columns;
        columns.int64("chainId")
            .int32("position")
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .int32("nStart")
            .int32("nEnd");
        columns.real("length").real("cumulativeLength").flag("chainIsClosed");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addElementChainsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementChainsQuery>());
}

} // namespace oor::query
