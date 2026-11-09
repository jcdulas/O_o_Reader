#include "ForceBlocks.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/results/r3_forces/ForceQueries.hpp"

namespace oor::query
{

namespace
{

const std::vector<std::string>& shellFields()
{
    static const std::vector<std::string> FIELDS = {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"};
    return FIELDS;
}

// shellFluxes: the membrane forces, the moments and the transverse shears of the shells, per unit length.
class ShellFluxesQuery final : public ResultQuery
{
public:
    ShellFluxesQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        const ElementRequest request = ElementRequest::from(context.arguments());
        if (!request.location.empty() && request.location != "center" && request.location != "corners")
            throw QueryError("location is None, \"center\" or \"corners\", not '" + request.location + "'");
        const ElementResolution resolution = resolveElements(context, shellResultantChain(), request,
                                                             "the force resultants of the shells", shellFields());
        ForceColumns columns;
        columns.location = true;
        columns.node = true;
        columns.frame = true;
        return forceTable(context, info(), resolution, shellFields(), columns, {"elementId", "nodeId"});
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "shellFluxes";
        info.family = "R3";
        info.resultQuery = true;
        info.description =
            "The force resultants of the shells per unit length: the membrane forces `nx ny nxy`, the bending moments "
            "`mx my "
            "mxy` and the transverse shears `qx qy`, at the centre or at the corners (`location`, `nodeId`), with the "
            "frame "
            "they are written in. They come from the force table; a shell without one gets them integrated from the "
            "stresses "
            "at its two fibres and its thickness (an estimate, marked approximate, with no transverse shear).";
        info.parameters = {
            parameter("elementIds", ParameterKind::IdList, "None", "the elements to list"),
            parameter("types", ParameterKind::TextList, "None", "the element types (\"ShellQuad4\", ...)"),
            parameter("location", ParameterKind::Text, "None", "None (all), \"center\" or \"corners\"")};
        info.defaultSort = "subcase, [step], elementId, nodeId";
        ColumnListBuilder columns;
        columns.int32("subcase")
            .step()
            .int32("elementId")
            .dictionary("type")
            .int32("propId")
            .dictionary("location")
            .int32("nodeId");
        columns.results("nx ny nxy mx my mxy qx qy").dictionary("frame");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addShellFluxesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ShellFluxesQuery>());
}

} // namespace oor::query
