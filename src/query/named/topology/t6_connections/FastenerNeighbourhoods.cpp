#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/analysis/geometry/ElementGeometry.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace oor::query
{

namespace
{

// The shells of the model: those at each grid point, and the area of each (computed when first asked).
class ShellIndex
{
public:
    explicit ShellIndex(const model::Model& model) : m_model(model)
    {
        const model::CardRegistry& registry = model::CardRegistry::instance();
        std::vector<base::EntityId> nodes;
        for (std::uint16_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTable* table = model.findTable(type);
            if (table == nullptr || analysis::dimensionOf(analysis::ElementTopology::shapeOf(type)) != 2)
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const auto index = static_cast<std::uint32_t>(m_shells.size());
                m_shells.push_back(Shell{model::CardLocation{type, row},
                                         static_cast<std::int32_t>(table->idAt(row)),
                                         {},
                                         std::numeric_limits<double>::quiet_NaN()});
                nodes.clear();
                analysis::ElementTopology::nodesOf(model, m_shells.back().location, nodes);
                m_shells.back().nodes = nodes;
                for (const base::EntityId node : nodes)
                    m_at[node].push_back(index);
            }
        }
    }

    [[nodiscard]] const std::vector<std::uint32_t>& at(base::EntityId node) const
    {
        static const std::vector<std::uint32_t> NONE;
        const auto found = m_at.find(node);
        return found != m_at.end() ? found->second : NONE;
    }
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::uint32_t shell) const { return m_shells[shell].nodes; }
    [[nodiscard]] std::int32_t id(std::uint32_t shell) const { return m_shells[shell].id; }
    [[nodiscard]] std::uint16_t type(std::uint32_t shell) const { return m_shells[shell].location.table; }
    [[nodiscard]] double area(std::uint32_t shell)
    {
        Shell& entry = m_shells[shell];
        if (std::isnan(entry.area))
            entry.area =
                analysis::ElementGeometry::of(m_model, entry.location, analysis::bit(analysis::Quantity::Area)).area;
        return entry.area;
    }

private:
    struct Shell
    {
        model::CardLocation location;
        std::int32_t id = 0;
        std::vector<base::EntityId> nodes;
        double area = 0.0;
    };
    const model::Model& m_model;
    std::vector<Shell> m_shells;
    std::unordered_map<base::EntityId, std::vector<std::uint32_t>> m_at;
};

// One side of a fastener: its card, its side and the grid points it is tied to (those of the shell it pierces).
struct FastenerSide
{
    std::int32_t eid = 0;
    std::string type;
    std::string side;
    std::vector<base::EntityId> nodes;
};

// fastenerNeighbourhoods: the shells around each side of each fastener, with their weights.
class FastenerNeighbourhoodsQuery final : public QueryBase
{
public:
    FastenerNeighbourhoodsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::int64_t rings = run.arguments.integerOr("rings", 1);
        if (rings < 1)
            throw QueryError(
                "fastenerNeighbourhoods: rings is 1 (the shells the fastener pierces) or more (each ring adds "
                "the shells that share a grid point with the ring before)");
        std::optional<std::vector<std::int64_t>> wanted = run.arguments.ids("fasteners");
        if (wanted)
            std::sort(wanted->begin(), wanted->end());
        const auto isWanted = [&](std::int64_t id)
        { return !wanted || std::binary_search(wanted->begin(), wanted->end(), id); };
        const model::CardRegistry& registry = model::CardRegistry::instance();

        // the sides of the fasteners: the grid points of the shell each side of a CFAST or CWELD pierces (GA and GB
        // of an ALIGN CWELD), the grid point of a CBUSH
        std::vector<FastenerSide> sides;
        const auto piercings = analysis::FastenerPiercings::of(model, run.context.pool());
        for (const analysis::PiercedFastener& fastener : piercings->fasteners())
        {
            if (!isWanted(fastener.eid))
                continue;
            const auto eid = static_cast<std::int32_t>(fastener.eid);
            const std::string type(registry.entry(fastener.card.table).info.name);
            if (fastener.form == analysis::FastenerForm::Align)
            {
                if (fastener.ga != 0)
                    sides.push_back(FastenerSide{eid, type, "A", {fastener.ga}});
                if (fastener.gb != 0)
                    sides.push_back(FastenerSide{eid, type, "B", {fastener.gb}});
                continue;
            }
            for (const analysis::FastenerPiercing& side : piercings->sidesOf(fastener))
                sides.push_back(FastenerSide{eid, type, std::string(1, side.side), side.nids});
        }
        if (const model::BushingTable* bushes = model.findTable<model::BushingTable>())
        {
            for (std::uint32_t row = 0; row < bushes->rows().rowCount(); ++row)
            {
                if (!bushes->rows().isAlive(row))
                    continue;
                const model::BushingCard card = bushes->card(row);
                if (!isWanted(card.id))
                    continue;
                const std::pair<char, base::EntityId> ends[2] = {{'A', card.nodeA}, {'B', card.nodeB}};
                for (const auto& [name, node] : ends)
                    if (node > 0)
                        sides.push_back(FastenerSide{static_cast<std::int32_t>(card.id),
                                                     std::string(model::BushingTable::NAME),
                                                     std::string(1, name),
                                                     {node}});
            }
        }
        std::stable_sort(sides.begin(), sides.end(),
                         [](const FastenerSide& one, const FastenerSide& other) { return one.eid < other.eid; });

        ShellIndex shells(model);
        std::vector<std::int32_t> fastener;
        std::vector<std::string> type;
        std::vector<std::string> side;
        std::vector<std::int32_t> eid;
        std::vector<std::int32_t> ring;
        std::vector<double> weight;
        for (const FastenerSide& entry : sides)
        {
            // ring 1: the shells whose grid points are all grid points of the side (the shell pierced, or the shell on
            // the face of grid points of a GRIDID side), else (a grid point of a CBUSH or an ALIGN CWELD) the shells
            // at the grid points of the side; each ring after adds the shells that share a grid point with the ring
            // before
            const std::set<base::EntityId> tied(entry.nodes.begin(), entry.nodes.end());
            std::set<std::uint32_t> touching;
            for (const base::EntityId node : tied)
                touching.insert(shells.at(node).begin(), shells.at(node).end());
            std::vector<std::uint32_t> first;
            for (const std::uint32_t shell : touching)
            {
                const std::span<const base::EntityId> own = shells.nodesOf(shell);
                if (std::all_of(own.begin(), own.end(), [&](base::EntityId node) { return tied.contains(node); }))
                    first.push_back(shell);
            }
            if (first.empty())
                first.assign(touching.begin(), touching.end());
            std::map<std::uint32_t, std::int32_t> ringOf;
            std::vector<std::uint32_t> ringShells = first;
            for (const std::uint32_t shell : first)
                ringOf.emplace(shell, 1);
            for (std::int32_t number = 2; number <= rings; ++number)
            {
                std::vector<std::uint32_t> next;
                for (const std::uint32_t shell : ringShells)
                    for (const base::EntityId node : shells.nodesOf(shell))
                        for (const std::uint32_t neighbour : shells.at(node))
                            if (ringOf.try_emplace(neighbour, number).second)
                                next.push_back(neighbour);
                ringShells = std::move(next);
            }
            if (ringOf.empty())
                continue;
            double total = 0.0;
            for (const auto& [shell, number] : ringOf)
                total += shells.area(shell);
            std::vector<std::uint32_t> order;
            for (const auto& [shell, number] : ringOf)
                order.push_back(shell);
            std::sort(order.begin(), order.end(),
                      [&](std::uint32_t one, std::uint32_t other) { return shells.id(one) < shells.id(other); });
            for (const std::uint32_t shell : order)
            {
                fastener.push_back(entry.eid);
                type.push_back(entry.type);
                side.push_back(entry.side);
                eid.push_back(shells.id(shell));
                ring.push_back(ringOf[shell]);
                weight.push_back(total > 0.0 ? shells.area(shell) / total : std::nan(""));
            }
        }
        TableBuilder builder;
        builder.int32("fastener", std::move(fastener));
        builder.dictionary("type", type);
        builder.dictionary("side", side);
        builder.int32("elementId", std::move(eid));
        builder.int32("ring", std::move(ring));
        builder.real("weight", std::move(weight));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "fastenerNeighbourhoods";
        info.family = "T6";
        info.description =
            "The shells around each side of each fastener, with their area fractions: ring 1 is the shells whose "
            "grid points are all grid points the side is tied to (for a CFAST or CWELD the grid points of the shell "
            "the side pierces, fastenerPiercings: that shell, or the shell on the face of grid points of a GRIDID "
            "side), or, when none is (the grid point GA or GB of a CBUSH or of an ALIGN CWELD), the shells at those "
            "grid points; each ring after adds the shells that share a grid point with "
            "the ring before, up to `rings`. `weight` is the area fraction of the shell: its area over the area of "
            "all the shells of the side (the weights of a side sum to 1), the weights of an area-weighted "
            "average over the side (one use: with the averages of readBlock, the mean fluxes of each plate "
            "around a fastener). "
            "`fasteners`: the CFAST, CWELD and CBUSH elements to list (all when None). One row per fastener, side and "
            "shell, by fastener, side and element ID.";
        info.parameters = {parameter("fasteners", ParameterKind::IdList, "None", "the fasteners (all when None)"),
                           parameter("rings", ParameterKind::Integer, "1", "the rings of shells around each side")};
        info.defaultSort = "fastener, side, elementId";
        ColumnListBuilder columns;
        columns.int32("fastener").dictionary("type").dictionary("side").int32("elementId").int32("ring").real("weight");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFastenerNeighbourhoodsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FastenerNeighbourhoodsQuery>());
}

} // namespace oor::query
