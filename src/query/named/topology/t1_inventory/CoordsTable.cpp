#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

const char* kindLetter(analysis::CoordinateKind kind)
{
    switch (kind)
    {
    case analysis::CoordinateKind::Rectangular:
        return "R";
    case analysis::CoordinateKind::Cylindrical:
        return "C";
    case analysis::CoordinateKind::Spherical:
        return "S";
    }
    return "R";
}

// coordsTable: the coordinate systems with their axes in the basic system, the depth of the chain that defines them
// and how many grid points and elements use each.
class CoordsTableQuery final : public QueryBase
{
public:
    CoordsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const analysis::SystemDefinitions definitions = analysis::readSystemDefinitions(model);
        if (definitions.empty())
            return emptyTable(info().defaultColumns());
        std::vector<base::EntityId> ids;
        for (const auto& entry : definitions)
            ids.push_back(entry.first);
        std::sort(ids.begin(), ids.end());

        // The grid points: how many use each system, and the system of each (for the depth of a CORD1x).
        std::map<base::EntityId, std::int64_t> cpUsers;
        std::map<base::EntityId, std::int64_t> cdUsers;
        const model::NodeTable* grids = model.findTable<model::NodeTable>();
        if (grids != nullptr)
        {
            for (std::uint32_t row = 0; row < grids->rows().rowCount(); ++row)
            {
                if (!grids->rows().isAlive(row))
                    continue;
                ++cpUsers[grids->positionFrameId(row)];
                ++cdUsers[grids->displacementFrameId(row)];
            }
        }
        // The elements that name a system (a material system, the system of a bush).
        std::map<base::EntityId, std::int64_t> elementUsers;
        {
            const auto references = analysis::ComponentCache::of(model).references(model);
            const model::CardRegistry& registry = model::CardRegistry::instance();
            for (const std::uint16_t type : typesOfCategory(model::CardCategory::Element))
            {
                const model::CardTable* table = model.findTable(type);
                if (table == nullptr)
                    continue;
                for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
                {
                    if (!table->rows().isAlive(row))
                        continue;
                    for (const analysis::ReferenceEdge& edge : references->from(model::CardLocation{type, row}))
                    {
                        if (edge.target == model::ReferenceTarget::Coordinate)
                            ++elementUsers[edge.id];
                    }
                }
            }
            (void)registry;
        }

        // The depth of the chain: the system itself and every system it is defined in, down to the basic one.
        std::map<base::EntityId, std::int64_t> depths;
        const auto depthOf = [&](auto&& self, base::EntityId cid, int guard) -> std::int64_t
        {
            if (cid == 0 || guard > 64)
                return 0;
            if (const auto found = depths.find(cid); found != depths.end())
                return found->second;
            const auto definition = definitions.find(cid);
            if (definition == definitions.end())
                return 0;
            std::int64_t deepest = 0;
            if (definition->second.form == analysis::SystemDefinition::Form::ThreePoints)
                deepest = self(self, definition->second.referenceFrameId, guard + 1);
            else
            {
                for (const base::EntityId node : definition->second.nodes)
                {
                    const auto location = model.locate(base::IdSpace::Node, node);
                    if (location && grids != nullptr && location->table == model::typeIndexOf<model::NodeTable>())
                        deepest = std::max(deepest, self(self, grids->positionFrameId(location->row), guard + 1));
                }
            }
            return depths[cid] = deepest + 1;
        };

        analysis::CoordinateTransform transform(model);
        const std::size_t count = ids.size();
        std::vector<std::string> types(count);
        std::vector<std::int32_t> definitionKind(count);
        std::vector<std::int32_t> referenceFrameIds(count, 0);
        std::vector<std::uint8_t> referenceFrameIdsValid(count, 0);
        std::array<std::vector<double>, 12> frame;
        for (auto& values : frame)
            values.assign(count, NOT_A_NUMBER);
        std::vector<std::int64_t> chainDepth(count);
        std::vector<std::int64_t> cps(count);
        std::vector<std::int64_t> cds(count);
        std::vector<std::int64_t> elementCounts(count);
        // the grid points that define a CORD1R, CORD1C or CORD1S (G1, G2, G3), null for a system given by coordinates
        std::array<std::vector<std::int32_t>, 3> points;
        std::array<std::vector<std::uint8_t>, 3> pointsValid;
        for (std::size_t item = 0; item < 3; ++item)
        {
            points[item].assign(count, 0);
            pointsValid[item].assign(count, 0);
        }
        for (std::size_t index = 0; index < count; ++index)
        {
            const base::EntityId cid = ids[index];
            const analysis::SystemDefinition& definition = definitions.at(cid);
            types[index] = kindLetter(definition.kind);
            definitionKind[index] = definition.form == analysis::SystemDefinition::Form::ThreeGrids ? 1 : 2;
            if (definition.form == analysis::SystemDefinition::Form::ThreePoints)
            {
                referenceFrameIds[index] = definition.referenceFrameId;
                referenceFrameIdsValid[index] = 1;
            }
            else
            {
                for (std::size_t item = 0; item < 3 && item < definition.nodes.size(); ++item)
                {
                    points[item][index] = definition.nodes[item];
                    pointsValid[item][index] = definition.nodes[item] != 0 ? 1 : 0;
                }
            }
            try
            {
                const analysis::CoordinateFrame& found = transform.frame(cid);
                const double values[12] = {found.origin.x,         found.origin.y,         found.origin.z,
                                           found.axes.column(0).x, found.axes.column(0).y, found.axes.column(0).z,
                                           found.axes.column(1).x, found.axes.column(1).y, found.axes.column(1).z,
                                           found.axes.column(2).x, found.axes.column(2).y, found.axes.column(2).z};
                for (std::size_t item = 0; item < 12; ++item)
                    frame[item][index] = values[item];
            }
            catch (const base::OorError&)
            {
                // A system that cannot be resolved (a loop, a missing point) keeps NaN axes: the validation reports it.
            }
            chainDepth[index] = depthOf(depthOf, cid, 0);
            const auto count1 = cpUsers.find(cid);
            const auto count2 = cdUsers.find(cid);
            const auto count3 = elementUsers.find(cid);
            cps[index] = count1 == cpUsers.end() ? 0 : count1->second;
            cds[index] = count2 == cdUsers.end() ? 0 : count2->second;
            elementCounts[index] = count3 == elementUsers.end() ? 0 : count3->second;
        }
        TableBuilder builder;
        builder.int32("frameId", std::vector<base::EntityId>(ids));
        builder.dictionary("type", types);
        builder.int32("definition", std::move(definitionKind));
        builder.int32("referenceFrameId", std::move(referenceFrameIds), std::move(referenceFrameIdsValid));
        static const char* const NAMES[12] = {"originX", "originY", "originZ", "iX", "iY", "iZ",
                                              "jX",      "jY",      "jZ",      "kX", "kY", "kZ"};
        for (std::size_t item = 0; item < 12; ++item)
            builder.real(NAMES[item], std::move(frame[item]));
        builder.int64("chainDepth", std::move(chainDepth));
        builder.int64("cpUserCount", std::move(cps));
        builder.int64("cdUserCount", std::move(cds));
        builder.int64("elementUserCount", std::move(elementCounts));
        static const char* const POINTS[3] = {"g1", "g2", "g3"};
        for (std::size_t item = 0; item < 3; ++item)
            builder.int32(POINTS[item], std::move(points[item]), std::move(pointsValid[item]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "coordsTable";
        info.family = "T1";
        info.description =
            "The coordinate systems: kind (R, C, S), how it is defined (1 by grid points, 2 by coordinates), the "
            "reference "
            "system, the origin and the axes in the basic system, the depth of the chain of systems it is defined in "
            "(1 for a "
            "system defined in the basic one), and the number of grid points that use it as position system and as "
            "displacement system and of elements that name it; g1 to g3 the grid points that define a CORD1R, CORD1C "
            "or "
            "CORD1S (null for a system given by coordinates): a solver that moves grid points derives its axes again "
            "from them.";
        info.defaultSort = "frameId";
        ColumnListBuilder columns;
        columns.int32("frameId").dictionary("type").int32("definition").int32("referenceFrameId");
        columns.reals("originX originY originZ iX iY iZ jX jY jZ kX kY kZ");
        columns.int64("chainDepth").int64("cpUserCount").int64("cdUserCount").int64("elementUserCount");
        columns.int32("g1").int32("g2").int32("g3");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addCoordsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<CoordsTableQuery>());
}

} // namespace oor::query
