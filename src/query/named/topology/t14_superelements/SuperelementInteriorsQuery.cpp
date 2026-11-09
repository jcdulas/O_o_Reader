#include "oor/analysis/superelements/PartBoundaries.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t14_superelements/SuperelementQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// superelementInteriors: what the list superelements of the main Bulk Data hold, their interior grid points or their
// elements.
class SuperelementInteriorsQuery final : public QueryBase
{
public:
    SuperelementInteriorsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "nodes");
        if (detail != "nodes" && detail != "elements")
            throw QueryError("superelementInteriors: detail is \"nodes\" or \"elements\", not \"" + detail + "\"");
        const analysis::PartBoundaries boundaries(run.model->root());
        const analysis::ListSuperelementPartition& partition = boundaries.listPartition();
        const auto onlyParts = run.arguments.ids("partIds");
        const auto wanted = [&](base::EntityId seid)
        { return !onlyParts || std::find(onlyParts->begin(), onlyParts->end(), seid) != onlyParts->end(); };
        TableBuilder builder;
        if (detail == "nodes")
        {
            std::vector<std::int32_t> seid;
            std::vector<std::int32_t> nid;
            std::vector<std::string> source;
            for (const analysis::InteriorPoint& point : partition.interiorPoints())
            {
                if (!wanted(point.seid))
                    continue;
                seid.push_back(point.seid);
                nid.push_back(point.nid);
                source.emplace_back(point.fromInteriorNodes ? "SuperelementInteriorNodes" : "Node");
            }
            builder.int32("partId", std::move(seid));
            builder.int32("nodeId", std::move(nid));
            builder.dictionary("source", source);
            return builder.finish();
        }
        const model::CardRegistry& registry = model::CardRegistry::instance();
        std::vector<std::int32_t> seid;
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        std::vector<std::string> source;
        for (const analysis::PartitionElement& element : partition.elements())
        {
            if (!wanted(element.seid))
                continue;
            seid.push_back(element.seid);
            eid.push_back(element.eid);
            type.emplace_back(registry.entry(element.type).info.name);
            source.emplace_back(element.fromElemTransfer ? "SuperelementElemTransfer" : "points");
        }
        builder.int32("partId", std::move(seid));
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.dictionary("source", source);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "superelementInteriors";
        info.family = "T14";
        info.description =
            "The interior of each list superelement of the main Bulk Data (format reference SESET, GRID SEID, SEELT), "
            "by "
            "superelement then ID. detail=\"nodes\" (the default): its interior grid points (partId, nodeId), source "
            "SuperelementInteriorNodes for a point an SESET lists (it takes precedence over the SEID of the GRID) "
            "or Node for the SEID of the GRID (or of GRDSET). detail=\"elements\": "
            "its elements (partId, elementId, type), source SuperelementElemTransfer for an element an SEELT gives it, "
            "points for an element on its interior points. A point or element of no list "
            "superelement belongs to the residual structure and has no row; the exterior points of each superelement "
            "are in partBoundaries. partIds keeps those superelements.";
        info.detailParameter = "detail";
        info.parameters = {
            parameter("detail", ParameterKind::Text, "\"nodes\"", "\"nodes\" or \"elements\""),
            parameter("partIds", ParameterKind::IdList, "None", "keep the interiors of these superelements")};
        info.defaultSort = "partId, then nodeId or elementId";
        ColumnListBuilder nodes;
        nodes.int32("partId").int32("nodeId").dictionary("source");
        info.columnSets.push_back(nodes.build("nodes"));
        ColumnListBuilder elements;
        elements.int32("partId").int32("elementId").dictionary("type").dictionary("source");
        info.columnSets.push_back(elements.build("elements"));
        return info;
    }
};

} // namespace

void addSuperelementInteriorsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SuperelementInteriorsQuery>());
}

} // namespace oor::query
