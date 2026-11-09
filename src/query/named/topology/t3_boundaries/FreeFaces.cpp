#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

// freeFaces: the faces of the solids that only one solid has: the skin.
class FreeFacesQuery final : public QueryBase
{
public:
    FreeFacesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto table = analysis::AdjacencyCache::of(model).faceTable(model, run.context.pool());
        const auto only = sortedIds(run.arguments.ids("elementIds"));
        const auto idOf = [&](const analysis::ElementRef& element)
        { return model.findTable(element.type)->idAt(element.row); };
        const auto isSolid = [](const analysis::ElementRef& element)
        { return analysis::dimensionOf(analysis::ElementTopology::shapeOf(element.type)) == 3; };

        // A face is free when exactly one selected solid has it.
        struct Free
        {
            std::uint32_t face;
            analysis::ElementRef owner;
        };
        std::vector<Free> faces;
        for (std::size_t face = 0; face < table->faceCount(); ++face)
        {
            std::size_t solids = 0;
            analysis::ElementRef owner;
            for (const analysis::ElementRef& element : table->elements(face))
            {
                if (!isSolid(element))
                    continue;
                if (only && !std::binary_search(only->begin(), only->end(), static_cast<std::int64_t>(idOf(element))))
                    continue;
                ++solids;
                owner = element;
            }
            if (solids == 1)
                faces.push_back(Free{static_cast<std::uint32_t>(face), owner});
        }
        const std::size_t count = faces.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        CardList owners;
        for (const Free& free : faces)
        {
            owners.types.push_back(free.owner.type);
            owners.rows.push_back(free.owner.row);
            owners.ids.push_back(idOf(free.owner));
        }
        ListColumns columns(run.model, owners, run.context);
        const std::vector<std::int32_t> pids = asInt32(columns.get("propId"));
        const NodePositions positions(model, run.context.pool());

        std::vector<std::int32_t> faceId(count);
        std::vector<std::int32_t> faceIndex(count);
        std::vector<std::int64_t> offsets(count + 1, 0);
        std::vector<std::int32_t> nodes;
        std::vector<double> area(count);
        std::array<std::vector<double>, 6> vectors;
        for (auto& values : vectors)
            values.assign(count, std::numeric_limits<double>::quiet_NaN());
        for (std::size_t index = 0; index < count; ++index)
        {
            const Free& free = faces[index];
            faceId[index] = static_cast<std::int32_t>(free.face);
            std::array<base::EntityId, 4> corners = table->faces()[free.face].nodes;
            faceIndex[index] = orientedFaceIndex(model, free.owner, table->faces()[free.face], corners);
            const std::size_t cornerCount = corners[3] == 0 ? 3 : 4;
            for (std::size_t corner = 0; corner < cornerCount; ++corner)
                nodes.push_back(corners[corner]);
            offsets[index + 1] = static_cast<std::int64_t>(nodes.size());
            const base::Vec3 a = positions.position(corners[0]);
            const base::Vec3 b = positions.position(corners[1]);
            const base::Vec3 c = positions.position(corners[2]);
            base::Vec3 vectorArea;
            base::Vec3 centre = a + b + c;
            if (cornerCount == 3)
                vectorArea = 0.5 * base::cross(b - a, c - a);
            else
            {
                const base::Vec3 d = positions.position(corners[3]);
                vectorArea = 0.5 * base::cross(c - a, d - b);
                centre += d;
            }
            centre = centre / static_cast<double>(cornerCount);
            area[index] = base::norm(vectorArea);
            const base::Vec3 normal = base::normalized(vectorArea);
            vectors[0][index] = normal.x;
            vectors[1][index] = normal.y;
            vectors[2][index] = normal.z;
            vectors[3][index] = centre.x;
            vectors[4][index] = centre.y;
            vectors[5][index] = centre.z;
        }
        std::vector<std::int32_t> eid(count);
        std::vector<std::uint8_t> pidValid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            eid[index] = owners.ids[index];
            pidValid[index] = pids[index] != 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("faceId", std::move(faceId));
        builder.int32("elementId", std::move(eid));
        builder.int32("propId", std::vector<std::int32_t>(pids), std::move(pidValid));
        builder.int32("faceIndex", std::move(faceIndex));
        builder.intList("nodes", std::move(offsets), std::move(nodes));
        builder.real("area", std::move(area));
        static const char* const NAMES[6] = {"normalX", "normalY", "normalZ", "cx", "cy", "cz"};
        for (std::size_t item = 0; item < 6; ++item)
            builder.real(NAMES[item], std::move(vectors[item]));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "freeFaces";
        info.family = "T3";
        info.description =
            "The skin of the solids: the faces that exactly one solid has. faceId is the position of the face in the "
            "face "
            "table, elementId and propId the solid that owns it, faceIndex the position (from 1) of the face in the "
            "face list "
            "of "
            "the element type (the template of the library: a tetrahedron's faces are opposite to G1, G2, G3, G4 in "
            "turn, "
            "a hexahedron's are G1-G4-G3-G2, G5-G6-G7-G8, G1-G2-G6-G5, G2-G3-G7-G6, G3-G4-G8-G7, G4-G1-G5-G8, a "
            "pentahedron's "
            "G1-G3-G2, G4-G5-G6, G1-G2-G5-G4, G2-G3-G6-G5, G3-G1-G4-G6, a pyramid's G1-G4-G3-G2 then the four "
            "triangles on "
            "G5), nodes in the order of the outward normal, area, the outward unit normal and the centre. With "
            "elementIds "
            "the faces are those of that selection of solids (its skin).";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the solids whose skin to find")};
        info.defaultSort = "faceId";
        ColumnListBuilder columns;
        columns.int32("faceId").int32("elementId").int32("propId").int32("faceIndex").intList("nodes").real("area");
        columns.vector3("normal", "XYZ");
        columns.reals("cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFreeFacesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FreeFacesQuery>());
}

} // namespace oor::query
