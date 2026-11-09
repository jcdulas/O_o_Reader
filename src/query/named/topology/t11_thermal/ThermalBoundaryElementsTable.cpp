#include "ThermalQuerySupport.hpp"
#include "oor/analysis/geometry/SurfaceSides.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

const char* kindName(analysis::SurfaceKind kind)
{
    switch (kind)
    {
    case analysis::SurfaceKind::Area:
        return "Area";
    case analysis::SurfaceKind::Line:
        return "Line";
    case analysis::SurfaceKind::Point:
        return "Point";
    case analysis::SurfaceKind::None:
        break;
    }
    return "";
}

// thermalBoundaryElementsTable: the surface elements CHBDYE, CHBDYG and CHBDYP.
class ThermalBoundaryElementsTableQuery final : public QueryBase
{
public:
    ThermalBoundaryElementsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const std::vector<std::uint16_t> types = {model::typeIndexOf<model::HeatSurfaceSideTable>(),
                                                  model::typeIndexOf<model::HeatSurfaceNodesTable>(),
                                                  model::typeIndexOf<model::HeatSurfacePropertyTable>()};
        const CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());
        const std::size_t count = list.size();
        const thermal::SurfaceElements surfaces(model, run.context.pool());

        std::vector<std::string> card(count);
        std::vector<std::string> surfaceType(count);
        std::vector<std::int32_t> pid(count, 0);
        std::vector<std::uint8_t> pidValid(count, 0);
        std::vector<std::int32_t> underlying(count, 0);
        std::vector<std::uint8_t> underlyingValid(count, 0);
        std::vector<std::int32_t> side(count, 0);
        std::vector<std::uint8_t> sideValid(count, 0);
        std::vector<std::vector<std::int32_t>> nodes(count);
        std::vector<double> area(count, NOT_A_NUMBER);
        std::vector<std::int32_t> radiationFront(count, 0);
        std::vector<std::int32_t> radiationBack(count, 0);
        std::vector<std::int32_t> viewFront(count, 0);
        std::vector<std::int32_t> viewBack(count, 0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::CardLocation where = list.location(index);
            card[index] = std::string(typeName(list.types[index]));
            const model::CardTable& table = *model.findTable(where.table);
            if (where.table == model::typeIndexOf<model::HeatSurfaceSideTable>())
            {
                const model::HeatSurfaceSideCard data =
                    static_cast<const model::HeatSurfaceSideTable&>(table).card(where.row);
                underlying[index] = data.conductionElemId;
                underlyingValid[index] = 1;
                side[index] = data.sideCode;
                sideValid[index] = 1;
                radiationFront[index] = data.frontRadiationPropId;
                radiationBack[index] = data.backRadiationPropId;
                viewFront[index] = data.frontViewId;
                viewBack[index] = data.backViewId;
            }
            else if (where.table == model::typeIndexOf<model::HeatSurfaceNodesTable>())
            {
                const model::HeatSurfaceNodesCard data =
                    static_cast<const model::HeatSurfaceNodesTable&>(table).card(where.row);
                surfaceType[index] = data.kind;
                radiationFront[index] = data.frontRadiationPropId;
                radiationBack[index] = data.backRadiationPropId;
                viewFront[index] = data.frontViewId;
                viewBack[index] = data.backViewId;
            }
            else
            {
                const model::HeatSurfacePropertyCard data =
                    static_cast<const model::HeatSurfacePropertyTable&>(table).card(where.row);
                surfaceType[index] = data.kind;
                pid[index] = data.propId;
                pidValid[index] = 1;
                radiationFront[index] = data.frontRadiationPropId;
                radiationBack[index] = data.backRadiationPropId;
                viewFront[index] = data.frontViewId;
                viewBack[index] = data.backViewId;
            }
            const analysis::SurfaceDescription description = analysis::describeSurface(model, where);
            if (surfaceType[index].empty())
                surfaceType[index] = kindName(description.kind); // the kind of the side of a CHBDYE
            nodes[index].assign(description.nodes.begin(), description.nodes.end());
            if (const thermal::SurfaceElement* surface = surfaces.find(list.ids[index]);
                surface != nullptr && surface->location == where)
                area[index] = surface->area;
        }
        const auto validIfNonzero = [](const std::vector<std::int32_t>& values)
        {
            std::vector<std::uint8_t> valid(values.size());
            for (std::size_t index = 0; index < values.size(); ++index)
                valid[index] = values[index] != 0 ? 1 : 0;
            return valid;
        };
        std::vector<std::uint8_t> surfaceTypeValid(count);
        for (std::size_t index = 0; index < count; ++index)
            surfaceTypeValid[index] = surfaceType[index].empty() ? 0 : 1;
        TableBuilder builder;
        builder.int32("elementId", std::vector<std::int32_t>(list.ids));
        builder.dictionary("type", card);
        builder.dictionary("surfaceType", surfaceType, std::move(surfaceTypeValid));
        builder.int32("propId", std::move(pid), std::move(pidValid));
        builder.int32("underlyingElementId", std::move(underlying), std::move(underlyingValid));
        builder.int32("side", std::move(side), std::move(sideValid));
        builder.intLists("nodes", nodes);
        builder.real("area", std::move(area));
        builder.int32("radiationMatIdFront", radiationFront, validIfNonzero(radiationFront));
        builder.int32("radiationMatIdBack", radiationBack, validIfNonzero(radiationBack));
        builder.int32("viewFront", viewFront, validIfNonzero(viewFront));
        builder.int32("viewBack", viewBack, validIfNonzero(viewBack));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "thermalBoundaryElementsTable";
        info.family = "T11";
        info.description =
            "The heat transfer surface elements: type is the card (CHBDYE, CHBDYG, CHBDYP) and surfaceType the type "
            "of "
            "its surface (AREA3 to AREA8, REV, REV1 of a CHBDYG; POINT, LINE, ELCYL, FTUBE, TUBE of a CHBDYP; AREA, "
            "LINE or "
            "POINT, the kind of the side, for a CHBDYE). propId is the PHBDY of a CHBDYP, underlyingElementId and side "
            "the "
            "conduction element and the side of a CHBDYE (null for the others). nodes are the grid points of the "
            "surface (a "
            "CHBDYE: the corners of its side, found from the element and the format reference side numbering). area is "
            "the area of "
            "the "
            "surface (the analysis geometry: the face of a CHBDYE or CHBDYG, AF of a POINT, AF times the length of a "
            "LINE; "
            "null where it is not defined, for instance when the conduction element is missing). The radiation "
            "materials "
            "(RADM) and view factors (VIEW) of the front and back faces are null when blank.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep these surface elements")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .dictionary("type")
            .dictionary("surfaceType")
            .int32("propId")
            .int32("underlyingElementId")
            .int32("side");
        columns.intList("nodes").real("area");
        columns.int32("radiationMatIdFront").int32("radiationMatIdBack").int32("viewFront").int32("viewBack");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addThermalBoundaryElementsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ThermalBoundaryElementsTableQuery>());
}

} // namespace oor::query
