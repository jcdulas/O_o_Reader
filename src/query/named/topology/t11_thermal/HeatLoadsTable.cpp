#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/SetSelection.hpp"
#include "oor/query/named/topology/t11_thermal/HeatLoads.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

namespace oor::query
{

namespace
{

// heatLoadsTable: the heat loads with the power each puts into its target.
class HeatLoadsTableQuery final : public QueryBase
{
public:
    HeatLoadsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::optional<std::vector<base::EntityId>> sets =
            askedSets(*run.model, run.arguments, model::LOAD_SELECTION);
        return finish(computeHeatLoads(run.model, run.context, sets));
    }

private:
    static std::shared_ptr<const QueryTable> finish(const std::vector<HeatLoad>& rows)
    {
        const std::size_t count = rows.size();
        std::vector<std::int32_t> sid(count);
        std::vector<std::string> card(count);
        std::vector<std::string> targetType(count);
        std::vector<std::int32_t> target(count);
        std::vector<std::uint8_t> targetValid(count);
        std::vector<double> flux(count);
        std::vector<double> extent(count);
        std::vector<double> power(count);
        std::vector<std::int32_t> control(count);
        std::vector<std::uint8_t> controlValid(count);
        std::vector<double> directionX(count);
        std::vector<double> directionY(count);
        std::vector<double> directionZ(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const HeatLoad& row = rows[index];
            sid[index] = row.sid;
            card[index] = row.card;
            targetType[index] = row.targetType;
            target[index] = row.target;
            targetValid[index] = row.targetValid ? 1 : 0;
            flux[index] = row.flux;
            extent[index] = row.extent;
            power[index] = row.power;
            control[index] = row.control;
            controlValid[index] = row.control != 0 ? 1 : 0;
            directionX[index] = row.direction.x;
            directionY[index] = row.direction.y;
            directionZ[index] = row.direction.z;
        }
        TableBuilder builder;
        builder.int32("setId", std::move(sid));
        builder.dictionary("card", card);
        builder.dictionary("targetType", targetType);
        builder.int32("targetId", std::move(target), std::move(targetValid));
        builder.real("flux", std::move(flux));
        builder.real("areaOrVolume", std::move(extent));
        builder.real("power", std::move(power));
        builder.int32("controlNode", std::move(control), std::move(controlValid));
        builder.real("directionX", std::move(directionX));
        builder.real("directionY", std::move(directionY));
        builder.real("directionZ", std::move(directionZ));
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "heatLoadsTable";
        info.family = "T11";
        info.description =
            "The heat loads, a row for each target, card the card type (HeatFluxSurfaceLoad1, HeatFluxNodalLoad2, "
            "HeatFluxBoundaryLoad3, HeatFluxNodeSetLoad, HeatVolumeSourceLoad, HeatFluxVectorLoad: a QBDY1, QBDY2, "
            "QBDY3, QHBDY, QVOL, QVECT): QBDY1 and QBDY3 (a uniform flux into each surface element they "
            "name), "
            "QBDY2 (the fluxes at the points of one surface element: flux is their mean over its corners), QHBDY (a "
            "flux into "
            "an area given by grid points: targetType node, targetId the first grid point, the area of the face), "
            "QVOL "
            "(heat generated in each element: flux is the rate per unit volume, power the rate times HGEN of the "
            "material "
            "times the volume) and QVECT (a flux vector from a distant source: power is the flux times the area times "
            "the "
            "absolute cosine between the unit flux direction, directionX, directionY, directionZ in the basic "
            "system, and "
            "the normal of the face; null when the flux comes from tables of time). flux is the magnitude of the card, "
            "areaOrVolume the area of the surface element or the volume of the element, power their product (the "
            "heat "
            "per unit time the load puts into the target; null where the geometry is missing). targetType is "
            "surfaceElement, element or node. setIds and subcases name the sets (a subcase selects the set its LOAD "
            "command "
            "names; a LOAD combination card is not expanded); with neither, every set.";
        info.parameters = {parameter("setIds", ParameterKind::IdList, "None", "the load sets to list"),
                           parameter("subcases", ParameterKind::IdList, "None", "the sets these subcases select")};
        info.defaultSort = "setId (then the order of the cards)";
        ColumnListBuilder columns;
        columns.int32("setId").dictionary("card").dictionary("targetType").int32("targetId");
        columns.reals("flux areaOrVolume power").int32("controlNode");
        columns.vector3("direction", "XYZ");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addHeatLoadsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<HeatLoadsTableQuery>());
}

} // namespace oor::query
