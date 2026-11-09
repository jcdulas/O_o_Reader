#include "oor/base/math/Vec3.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

namespace
{

constexpr double RADIANS_TO_DEGREES = 180.0 / 3.14159265358979323846;

// featureEdges: the edges between two shells where the normals make an angle above a threshold.
class FeatureEdgesQuery final : public QueryBase
{
public:
    FeatureEdgesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const double threshold = run.arguments.realOr("angle", 30.0);
        const ShellSet shells(run, run.arguments.ids("elementIds"), ShellNeeds{true, false, false});
        const ShellEdges edges(run, shells);
        const NodePositions positions(*run.model, run.context.pool());
        std::vector<std::int32_t> edgeId;
        std::vector<std::int32_t> n1;
        std::vector<std::int32_t> n2;
        std::vector<std::int32_t> eid1;
        std::vector<std::int32_t> eid2;
        std::vector<double> angle;
        std::vector<double> length;
        for (const ShellEdge& edge : edges.edges())
        {
            if (edge.count != 2)
                continue;
            const analysis::EdgeKey key = edges.key(edge);
            const auto pair = edges.shellsOf(edge);
            base::Vec3 first = shells.normal(pair[0]);
            base::Vec3 second = shells.normal(pair[1]);
            // Two shells that run the edge the same way are oriented against each other (normalFlips): the angle is
            // the one between the surfaces, so one normal is turned back first.
            if (shells.runsForward(pair[0], key.first, key.second) ==
                shells.runsForward(pair[1], key.first, key.second))
                second = -second;
            const double cosine = std::clamp(base::dot(first, second), -1.0, 1.0);
            const double degrees = std::acos(cosine) * RADIANS_TO_DEGREES;
            if (!(degrees > threshold))
                continue;
            edgeId.push_back(static_cast<std::int32_t>(edge.edge));
            n1.push_back(key.first);
            n2.push_back(key.second);
            eid1.push_back(shells.id(pair[0]));
            eid2.push_back(shells.id(pair[1]));
            angle.push_back(degrees);
            length.push_back(positions.distance(key.first, key.second));
        }
        TableBuilder builder;
        builder.int32("edgeId", std::move(edgeId));
        builder.int32("n1", std::move(n1));
        builder.int32("n2", std::move(n2));
        builder.int32("elementId1", std::move(eid1));
        builder.int32("elementId2", std::move(eid2));
        builder.real("angle", std::move(angle));
        builder.real("length", std::move(length));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "featureEdges";
        info.family = "T3";
        info.description =
            "The edges between exactly two shells where the angle between the normals exceeds `angle` degrees (a sharp "
            "bend; the angle of a flat joint is 0). Two shells that are oriented against each other (see normalFlips) "
            "have "
            "one normal turned back first, so the angle is the bend of the surface and not an orientation error. "
            "elementId1 "
            "< "
            "elementId2. With elementIds only those shells are looked at.";
        info.parameters = {parameter("angle", ParameterKind::Real, "30.0", "the threshold, in degrees"),
                           parameter("elementIds", ParameterKind::IdList, "None", "only these elements are looked at")};
        info.defaultSort = "edgeId";
        ColumnListBuilder columns;
        columns.int32("edgeId")
            .int32("n1")
            .int32("n2")
            .int32("elementId1")
            .int32("elementId2")
            .real("angle")
            .real("length");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addFeatureEdgesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<FeatureEdgesQuery>());
}

} // namespace oor::query
