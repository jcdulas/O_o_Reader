#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/ShellEdges.hpp"
#include "oor/query/named/topology/t3_boundaries/BoundaryQueries.hpp"

#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// propertyBoundaries: the edges between two shells that differ by property, thickness or material.
class PropertyBoundariesQuery final : public QueryBase
{
public:
    PropertyBoundariesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string by = run.arguments.textOr("by", "property");
        if (by != "property" && by != "thickness" && by != "material")
            throw QueryError("propertyBoundaries: by is \"property\", \"thickness\" or \"material\", not \"" + by +
                             "\"");
        ShellNeeds needs;
        needs.thickness = by == "thickness";
        needs.material = by == "material";
        const ShellSet shells(run, run.arguments.ids("elementIds"), needs);
        const ShellEdges edges(run, shells);
        const NodePositions positions(*run.model, run.context.pool());
        const auto value = [&](std::size_t shell) -> double
        {
            if (by == "property")
                return shells.property(shell);
            if (by == "thickness")
                return shells.thickness(shell);
            return shells.material(shell);
        };
        std::vector<std::int32_t> edgeId;
        std::vector<std::int32_t> n1;
        std::vector<std::int32_t> n2;
        std::vector<std::int32_t> eid1;
        std::vector<std::int32_t> eid2;
        std::vector<std::int32_t> pid1;
        std::vector<std::int32_t> pid2;
        std::vector<double> value1;
        std::vector<double> value2;
        std::vector<double> length;
        for (const ShellEdge& edge : edges.edges())
        {
            if (edge.count != 2)
                continue;
            const auto pair = edges.shellsOf(edge);
            const double first = value(pair[0]);
            const double second = value(pair[1]);
            bool differs = first != second;
            if (by == "thickness")
            {
                // Thicknesses are reals: equal to rounding is equal. A thickness that is not defined differs from one
                // that is.
                const double scale = std::max({1.0, std::fabs(first), std::fabs(second)});
                const bool bothUndefined = std::isnan(first) && std::isnan(second);
                differs = !bothUndefined &&
                          (std::isnan(first) != std::isnan(second) || std::fabs(first - second) > 1e-9 * scale);
            }
            if (!differs)
                continue;
            const analysis::EdgeKey key = edges.key(edge);
            edgeId.push_back(static_cast<std::int32_t>(edge.edge));
            n1.push_back(key.first);
            n2.push_back(key.second);
            eid1.push_back(shells.id(pair[0]));
            eid2.push_back(shells.id(pair[1]));
            pid1.push_back(shells.property(pair[0]));
            pid2.push_back(shells.property(pair[1]));
            value1.push_back(first);
            value2.push_back(second);
            length.push_back(positions.distance(key.first, key.second));
        }
        std::vector<std::uint8_t> valid1(pid1.size());
        std::vector<std::uint8_t> valid2(pid2.size());
        for (std::size_t index = 0; index < pid1.size(); ++index)
        {
            valid1[index] = pid1[index] != 0 ? 1 : 0;
            valid2[index] = pid2[index] != 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("edgeId", std::move(edgeId));
        builder.int32("n1", std::move(n1));
        builder.int32("n2", std::move(n2));
        builder.int32("elementId1", std::move(eid1));
        builder.int32("elementId2", std::move(eid2));
        builder.int32("propId1", std::move(pid1), std::move(valid1));
        builder.int32("propId2", std::move(pid2), std::move(valid2));
        builder.real("value1", std::move(value1));
        builder.real("value2", std::move(value2));
        builder.real("length", std::move(length));
        (void)NOT_A_NUMBER;
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "propertyBoundaries";
        info.family = "T3";
        info.description =
            "The edges between exactly two shells that differ by the quantity `by`: the property (value1 and value2 "
            "are the "
            "property IDs), the thickness of the element, or the main material (the one of its property). elementId1 < "
            "elementId2. "
            "With "
            "elementIds only those shells are looked at. Two thicknesses are the same when they agree to 1e-9 of "
            "their "
            "size.";
        info.parameters = {
            parameter("by", ParameterKind::Text, "\"property\"", "\"property\", \"thickness\" or \"material\""),
            parameter("elementIds", ParameterKind::IdList, "None", "only these elements are looked at")};
        info.defaultSort = "edgeId";
        ColumnListBuilder columns;
        columns.int32("edgeId")
            .int32("n1")
            .int32("n2")
            .int32("elementId1")
            .int32("elementId2")
            .int32("propId1")
            .int32("propId2");
        columns.reals("value1 value2 length");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPropertyBoundariesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PropertyBoundariesQuery>());
}

} // namespace oor::query
