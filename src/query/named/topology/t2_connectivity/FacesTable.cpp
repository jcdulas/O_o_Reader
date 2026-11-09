#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"

#include <algorithm>
#include <array>

namespace oor::query
{

namespace
{

// facesTable: the faces of the solid elements, the solids on them, and free or internal.
class FacesTableQuery final : public QueryBase
{
public:
    FacesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto table = analysis::AdjacencyCache::of(model).faceTable(model, run.context.pool());
        const NodePositions positions(model, run.context.pool());
        const auto only = sortedIds(run.arguments.ids("elementIds"));
        const auto isSolid = [](const analysis::ElementRef& element)
        { return analysis::dimensionOf(analysis::ElementTopology::shapeOf(element.type)) == 3; };
        const auto idOf = [&](const analysis::ElementRef& element)
        { return model.findTable(element.type)->idAt(element.row); };

        std::vector<std::uint32_t> chosen;
        for (std::size_t face = 0; face < table->faceCount(); ++face)
        {
            bool any = false;
            bool selected = !only;
            for (const analysis::ElementRef& element : table->elements(face))
            {
                if (!isSolid(element))
                    continue;
                any = true;
                if (only && std::binary_search(only->begin(), only->end(), static_cast<std::int64_t>(idOf(element))))
                    selected = true;
            }
            if (any && selected)
                chosen.push_back(static_cast<std::uint32_t>(face));
        }
        const std::size_t count = chosen.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        std::vector<std::int32_t> faceId(count);
        std::vector<std::int64_t> nodeCount(count);
        std::vector<std::int64_t> nodeOffsets(count + 1, 0);
        std::vector<std::int64_t> elementOffsets(count + 1, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const analysis::FaceKey key = table->faces()[chosen[index]];
            nodeCount[index] = key.nodes[3] == 0 ? 3 : 4;
            nodeOffsets[index + 1] = nodeOffsets[index] + nodeCount[index];
            std::int64_t solids = 0;
            for (const analysis::ElementRef& element : table->elements(chosen[index]))
                solids += isSolid(element) ? 1 : 0;
            elementOffsets[index + 1] = elementOffsets[index] + solids;
        }
        std::vector<std::int32_t> nodes(static_cast<std::size_t>(nodeOffsets[count]));
        std::vector<std::int32_t> elements(static_cast<std::size_t>(elementOffsets[count]));
        std::vector<double> area(count);
        std::vector<std::int64_t> elementCount(count);
        std::vector<std::string> kind(count);
        run.context.parallel(count, 4096,
                             [&](base::IndexRange range)
                             {
                                 for (std::size_t index = range.begin; index < range.end; ++index)
                                 {
                                     const std::size_t face = chosen[index];
                                     faceId[index] = static_cast<std::int32_t>(face);
                                     std::size_t at = static_cast<std::size_t>(elementOffsets[index]);
                                     const std::size_t first = at;
                                     std::optional<analysis::ElementRef> owner;
                                     for (const analysis::ElementRef& element : table->elements(face))
                                     {
                                         if (!isSolid(element))
                                             continue;
                                         elements[at++] = idOf(element);
                                         if (!owner)
                                             owner = element;
                                     }
                                     std::sort(elements.begin() + static_cast<std::ptrdiff_t>(first),
                                               elements.begin() + static_cast<std::ptrdiff_t>(at));
                                     elementCount[index] = static_cast<std::int64_t>(at - first);
                                     kind[index] = at - first == 1 ? "free" : "internal";
                                     // The corners in the order of the outward normal, as the first solid on the face
                                     // lists them.
                                     std::array<base::EntityId, 4> corners = table->faces()[face].nodes;
                                     if (owner)
                                         (void)orientedFaceOf(model, *owner, table->faces()[face], corners);
                                     const std::size_t cornerCount = static_cast<std::size_t>(nodeCount[index]);
                                     for (std::size_t corner = 0; corner < cornerCount; ++corner)
                                         nodes[static_cast<std::size_t>(nodeOffsets[index]) + corner] = corners[corner];
                                     const base::Vec3 a = positions.position(corners[0]);
                                     const base::Vec3 b = positions.position(corners[1]);
                                     const base::Vec3 c = positions.position(corners[2]);
                                     if (cornerCount == 3)
                                         area[index] = 0.5 * base::norm(base::cross(b - a, c - a));
                                     else
                                     {
                                         const base::Vec3 d = positions.position(corners[3]);
                                         area[index] = 0.5 * base::norm(base::cross(c - a, d - b));
                                     }
                                 }
                             });
        TableBuilder builder;
        builder.int32("faceId", std::move(faceId));
        builder.int64("nodeCount", std::move(nodeCount));
        builder.intList("nodes", std::move(nodeOffsets), std::move(nodes));
        builder.real("area", std::move(area));
        builder.int64("elementCount", std::move(elementCount));
        builder.intList("elements", std::move(elementOffsets), std::move(elements));
        builder.dictionary("kind", kind);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "facesTable";
        info.family = "T2";
        info.description =
            "The faces of the solid elements: the corner nodes in the order that gives the outward normal (as the "
            "first solid "
            "on the face lists them), the area, the solids on the face and their number, and the kind: free (one "
            "solid) or "
            "internal (two). A face that only shells have is not listed. faceId is the position (from 0) of the face "
            "in the "
            "sorted table of all the faces of the model.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep the faces of these solids")};
        info.defaultSort = "faceId";
        ColumnListBuilder columns;
        columns.int32("faceId")
            .int64("nodeCount")
            .intList("nodes")
            .real("area")
            .int64("elementCount")
            .intList("elements");
        columns.dictionary("kind");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFacesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FacesTableQuery>());
}

} // namespace oor::query
