#include "ForceBlocks.hpp"
#include "oor/query/named/results/r3_forces/ForceQueries.hpp"

namespace oor::query
{

namespace
{

// elementForcesTable: the forces and moments of the one-dimensional elements.
class ElementForcesTableQuery final : public ResultQuery
{
public:
    ElementForcesTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        ElementRequest request = ElementRequest::from(context.arguments());
        const ElementResolution resolution =
            resolveElements(context, oneDimensionalForceChain(), request, "the forces of the one-dimensional elements",
                            oneDimensionalFields());
        ForceColumns columns;
        columns.location = true;
        columns.station = true;
        return forceTable(context, info(), resolution, oneDimensionalFields(), columns, {"elementId", "station"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementForcesTable";
        info.family = "R3";
        info.resultQuery = true;
        info.description =
            "The forces and moments of the one-dimensional elements (rods, bars, beams, bends): one row for each "
            "subcase, "
            "step, element and place along it (`location`: `center` for a rod, `endA` and `endB` for a bar, the ends "
            "and "
            "`station` for a beam; `station` is x/L), with the axial force, the two shears, the torque and the two "
            "bending "
            "moments, in the element system. A rod without a force table gets its force from its stress and the area "
            "of its "
            "property (`table.sources` and `withSource` say which way gave each row).";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
                           parameter("types", ParameterKind::TextList, "None", "the element types (\"Bar2\", ...)")};
        info.defaultSort = "subcase, [step], elementId, station";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .dictionary("location")
            .real("station");
        columns.results("axial shear1 shear2 torque moment1 moment2");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addElementForcesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementForcesTableQuery>());
}

} // namespace oor::query
