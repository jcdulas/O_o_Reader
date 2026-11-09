#include "oor/analysis/adjacency/AdjacencyCache.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/Topology.hpp"
#include "oor/query/named/topology/t2_connectivity/ConnectivityQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

// The kind of an edge: by the shells on it (one: free, two: manifold, more: non-manifold), else a line when only 1D
// elements carry it; an edge that only solids have is none of these.
constexpr const char* FREE = "free";
constexpr const char* MANIFOLD = "manifold";
constexpr const char* NON_MANIFOLD = "nonManifold";
constexpr const char* LINE = "line";

// edgesTable: the unique edges of the elements with their length, the elements on them and their kind.
class EdgesTableQuery final : public QueryBase
{
public:
    EdgesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto table = analysis::AdjacencyCache::of(model).edgeTable(model, run.context.pool());
        const NodePositions positions(model, run.context.pool());
        const auto onlyElements = sortedIds(run.arguments.ids("elementIds"));
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        std::vector<std::uint16_t> onlyTypes;
        if (!typeNames.empty())
            onlyTypes = typesNamed(typeNames, model::CardCategory::Element, "types");
        std::sort(onlyTypes.begin(), onlyTypes.end());

        const std::size_t edgeCount = table->edgeCount();
        const auto elementId = [&](const analysis::ElementRef& element) -> std::int32_t
        { return model.findTable(element.type)->idAt(element.row); };

        // The edges that have a selected element.
        std::vector<std::uint32_t> chosen;
        for (std::size_t edge = 0; edge < edgeCount; ++edge)
        {
            bool selected = !onlyElements && onlyTypes.empty();
            for (const analysis::ElementRef& element : table->elements(edge))
            {
                if (selected)
                    break;
                const bool idOk = !onlyElements || std::binary_search(onlyElements->begin(), onlyElements->end(),
                                                                      static_cast<std::int64_t>(elementId(element)));
                const bool typeOk =
                    onlyTypes.empty() || std::binary_search(onlyTypes.begin(), onlyTypes.end(), element.type);
                selected = idOk && typeOk;
            }
            if (selected)
                chosen.push_back(static_cast<std::uint32_t>(edge));
        }
        const std::size_t count = chosen.size();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        std::vector<std::int64_t> offsets(count + 1, 0);
        for (std::size_t index = 0; index < count; ++index)
            offsets[index + 1] = offsets[index] + static_cast<std::int64_t>(table->elements(chosen[index]).size());
        std::vector<std::int32_t> edgeId(count);
        std::vector<std::int32_t> n1(count);
        std::vector<std::int32_t> n2(count);
        std::vector<std::int32_t> mid(count, 0);
        std::vector<double> length(count);
        std::vector<std::int64_t> elementCount(count);
        std::vector<std::int32_t> members(static_cast<std::size_t>(offsets[count]));
        std::vector<std::string> kind(count);
        std::vector<std::uint8_t> kindValid(count, 1);
        run.context.parallel(count, 8192,
                             [&](base::IndexRange range)
                             {
                                 std::vector<base::EntityId> nodes;
                                 for (std::size_t index = range.begin; index < range.end; ++index)
                                 {
                                     const std::size_t edge = chosen[index];
                                     const analysis::EdgeKey key = table->edges()[edge];
                                     edgeId[index] = static_cast<std::int32_t>(edge);
                                     n1[index] = key.first;
                                     n2[index] = key.second;
                                     length[index] = positions.distance(key.first, key.second);
                                     const auto elements = table->elements(edge);
                                     elementCount[index] = static_cast<std::int64_t>(elements.size());
                                     const std::size_t start = static_cast<std::size_t>(offsets[index]);
                                     std::size_t shells = 0;
                                     bool lines = false;
                                     for (std::size_t item = 0; item < elements.size(); ++item)
                                     {
                                         const analysis::ElementRef& element = elements[item];
                                         members[start + item] = model.findTable(element.type)->idAt(element.row);
                                         const int dimension =
                                             analysis::dimensionOf(analysis::ElementTopology::shapeOf(element.type));
                                         shells += dimension == 2 ? 1 : 0;
                                         lines = lines || dimension == 1;
                                         if (mid[index] == 0 && dimension >= 2)
                                         {
                                             nodes.clear();
                                             analysis::ElementTopology::nodesOf(*model.findTable(element.type),
                                                                                element.type, element.row, nodes);
                                             std::size_t first = nodes.size();
                                             std::size_t second = nodes.size();
                                             for (std::size_t position = 0; position < nodes.size(); ++position)
                                             {
                                                 if (!isCornerPosition(element.type, position))
                                                     continue;
                                                 if (nodes[position] == key.first && first == nodes.size())
                                                     first = position;
                                                 else if (nodes[position] == key.second && second == nodes.size())
                                                     second = position;
                                             }
                                             if (first < nodes.size() && second < nodes.size())
                                             {
                                                 const std::vector<std::int32_t> asIds(nodes.begin(), nodes.end());
                                                 mid[index] = midSideNodeOf(element.type, asIds, first, second);
                                             }
                                         }
                                     }
                                     std::sort(members.begin() + static_cast<std::ptrdiff_t>(start),
                                               members.begin() + static_cast<std::ptrdiff_t>(start + elements.size()));
                                     if (shells == 1)
                                         kind[index] = FREE;
                                     else if (shells == 2)
                                         kind[index] = MANIFOLD;
                                     else if (shells > 2)
                                         kind[index] = NON_MANIFOLD;
                                     else if (lines)
                                         kind[index] = LINE;
                                     else
                                         kindValid[index] = 0;
                                 }
                             });
        std::vector<std::uint8_t> midValid(count);
        for (std::size_t index = 0; index < count; ++index)
            midValid[index] = mid[index] != 0 ? 1 : 0;
        TableBuilder builder;
        builder.int32("edgeId", std::move(edgeId));
        builder.int32("n1", std::move(n1));
        builder.int32("n2", std::move(n2));
        builder.int32("midNode", std::move(mid), std::move(midValid));
        builder.real("length", std::move(length));
        builder.int64("elementCount", std::move(elementCount));
        builder.intList("elements", std::move(offsets), std::move(members));
        builder.dictionary("kind", kind, std::move(kindValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "edgesTable";
        info.family = "T2";
        info.description =
            "The unique edges of the elements: the two corner nodes (n1 < n2), the mid-side node of a quadratic "
            "element, the "
            "length, the elements on the edge and their number, and the kind: free (one shell), manifold (two shells), "
            "nonManifold (three or more) or line (only 1D elements); null for an edge that only solids have. edgeId "
            "is the "
            "position (from 0) of the edge in the sorted table of all the edges of the model, so that the same edge "
            "has the "
            "same id in every query of the family T3.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep the edges of these elements"),
                           parameter("types", ParameterKind::TextList, "None", "keep the edges of these card types")};
        info.defaultSort = "edgeId (by n1, n2)";
        ColumnListBuilder columns;
        columns.int32("edgeId")
            .int32("n1")
            .int32("n2")
            .int32("midNode")
            .real("length")
            .int64("elementCount")
            .intList("elements");
        columns.dictionary("kind");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addEdgesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<EdgesTableQuery>());
}

} // namespace oor::query
