#include "ThermalQuerySupport.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/model/cards/thermal/radiationCavity/RadiationCavityTable.hpp"
#include "oor/model/cards/thermal/radiationCavityList/RadiationCavityListTable.hpp"
#include "oor/model/cards/thermal/radiationCavitySet/RadiationCavitySetTable.hpp"
#include "oor/model/cards/thermal/radiationProp/RadiationPropTable.hpp"
#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Row
{
    std::int32_t eid = 0;
    bool eidValid = true;
    int rank = 0; // the card, in the order the table lists them
    const char* card = "";
    std::int32_t ambient = 0;
    double viewFactor = NOT_A_NUMBER;
    std::int32_t control = 0;
    std::int32_t material = 0;
    double absorptivity = NOT_A_NUMBER;
    double emissivity = NOT_A_NUMBER;
    std::int32_t cavity = 0;
};

// The radiation material of the front face of a surface element (its back face when the front has none).
base::EntityId radiationMaterialOf(const model::Model& model, model::CardLocation where)
{
    const model::CardTable& table = *model.findTable(where.table);
    base::EntityId front = 0;
    base::EntityId back = 0;
    if (where.table == model::typeIndexOf<model::HeatSurfaceSideTable>())
    {
        const model::HeatSurfaceSideCard card =
            static_cast<const model::HeatSurfaceSideTable&>(table).card(where.row);
        front = card.frontRadiationPropId;
        back = card.backRadiationPropId;
    }
    else if (where.table == model::typeIndexOf<model::HeatSurfaceNodesTable>())
    {
        const model::HeatSurfaceNodesCard card =
            static_cast<const model::HeatSurfaceNodesTable&>(table).card(where.row);
        front = card.frontRadiationPropId;
        back = card.backRadiationPropId;
    }
    else if (where.table == model::typeIndexOf<model::HeatSurfacePropertyTable>())
    {
        const model::HeatSurfacePropertyCard card =
            static_cast<const model::HeatSurfacePropertyTable&>(table).card(where.row);
        front = card.frontRadiationPropId;
        back = card.backRadiationPropId;
    }
    return front != 0 ? front : back;
}

// radiationTable: the surface elements that radiate: those of a RADBC (radiation to an ambient point) and those of a
// RADLST (the elements of a cavity), with the radiation material of the element, the ambient element of a RADCAV and
// the cavities of a RADSET.
class RadiationTableQuery final : public QueryBase
{
public:
    RadiationTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const thermal::SurfaceElements surfaces(model, run.context.pool());
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("elementIds"));
        const auto asked = [&](std::int32_t eid)
        { return !wanted || std::binary_search(wanted->begin(), wanted->end(), static_cast<std::int64_t>(eid)); };

        std::vector<Row> rows;
        // The material of a surface element, then what the RADM gives.
        const auto fillMaterial = [&](Row& row)
        {
            const thermal::SurfaceElement* surface = surfaces.find(row.eid);
            row.material = surface != nullptr ? radiationMaterialOf(model, surface->location) : 0;
            if (row.material == 0)
                return;
            const auto found = model.locateInType(model::typeIndexOf<model::RadiationPropTable>(), row.material);
            if (!found)
                return;
            const model::RadiationPropCard radm =
                static_cast<const model::RadiationPropTable&>(*model.findTable(found->table)).card(found->row);
            row.absorptivity = radm.absorptivity.value_or(NOT_A_NUMBER);
            row.emissivity = radm.emissivity.empty() ? NOT_A_NUMBER : radm.emissivity.front();
        };
        if (const auto* table = model.findTable<model::RadiationToSpaceTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at))
                    continue;
                const model::RadiationToSpaceCard card = table->card(at);
                for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, surfaces.ids()))
                {
                    if (!asked(eid))
                        continue;
                    Row row;
                    row.eid = eid;
                    row.rank = 0;
                    row.card = "RadiationToSpace";
                    row.ambient = card.ambientNode;
                    row.viewFactor = card.ambientViewFactor;
                    row.control = card.controlNode;
                    fillMaterial(row);
                    rows.push_back(row);
                }
            }
        }
        if (const auto* table = model.findTable<model::RadiationCavityListTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at))
                    continue;
                const model::RadiationCavityListCard card = table->card(at);
                for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, surfaces.ids()))
                {
                    if (!asked(eid))
                        continue;
                    Row row;
                    row.eid = eid;
                    row.rank = 1;
                    row.card = "RadiationCavityList";
                    row.cavity = card.cavityId;
                    fillMaterial(row);
                    rows.push_back(row);
                }
            }
        }
        if (const auto* table = model.findTable<model::RadiationCavityTable>())
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at))
                    continue;
                const model::RadiationCavityCard card = table->card(at);
                if (card.ambientElemId == 0 || !asked(card.ambientElemId))
                    continue;
                Row row;
                row.eid = card.ambientElemId;
                row.rank = 2;
                row.card = "RadiationCavity";
                row.cavity = card.id;
                fillMaterial(row);
                rows.push_back(row);
            }
        }
        if (const auto* table = model.findTable<model::RadiationCavitySetTable>(); table != nullptr && !wanted)
        {
            for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
            {
                if (!table->rows().isAlive(at))
                    continue;
                for (const base::EntityId cavity : table->card(at).cavities)
                {
                    Row row;
                    row.eidValid = false;
                    row.rank = 3;
                    row.card = "RadiationCavitySet";
                    row.cavity = cavity;
                    rows.push_back(row);
                }
            }
        }
        std::stable_sort(rows.begin(), rows.end(),
                         [](const Row& left, const Row& right)
                         {
                             if (left.eidValid != right.eidValid)
                                 return left.eidValid;
                             if (left.eid != right.eid)
                                 return left.eid < right.eid;
                             return left.rank < right.rank;
                         });
        const std::size_t count = rows.size();
        std::vector<std::int32_t> eid(count);
        std::vector<std::uint8_t> eidValid(count);
        std::vector<std::string> card(count);
        std::vector<std::int32_t> ambient(count);
        std::vector<std::uint8_t> ambientValid(count);
        std::vector<double> viewFactor(count);
        std::vector<std::int32_t> control(count);
        std::vector<std::uint8_t> controlValid(count);
        std::vector<std::int32_t> material(count);
        std::vector<std::uint8_t> materialValid(count);
        std::vector<double> absorptivity(count);
        std::vector<double> emissivity(count);
        std::vector<std::int32_t> cavity(count);
        std::vector<std::uint8_t> cavityValid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const Row& row = rows[index];
            eid[index] = row.eid;
            eidValid[index] = row.eidValid ? 1 : 0;
            card[index] = row.card;
            ambient[index] = row.ambient;
            ambientValid[index] = row.ambient != 0 ? 1 : 0;
            viewFactor[index] = row.viewFactor;
            control[index] = row.control;
            controlValid[index] = row.control != 0 ? 1 : 0;
            material[index] = row.material;
            materialValid[index] = row.material != 0 ? 1 : 0;
            absorptivity[index] = row.absorptivity;
            emissivity[index] = row.emissivity;
            cavity[index] = row.cavity;
            cavityValid[index] = row.cavity != 0 ? 1 : 0;
        }
        TableBuilder builder;
        builder.int32("elementId", std::move(eid), std::move(eidValid));
        builder.dictionary("card", card);
        builder.int32("ambientNode", std::move(ambient), std::move(ambientValid));
        builder.real("viewFactor", std::move(viewFactor));
        builder.int32("controlNode", std::move(control), std::move(controlValid));
        builder.int32("radiationMatId", std::move(material), std::move(materialValid));
        builder.real("absorptivity", std::move(absorptivity));
        builder.real("emissivity", std::move(emissivity));
        builder.int32("cavity", std::move(cavity), std::move(cavityValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "radiationTable";
        info.family = "T11";
        info.description =
            "The radiating surface elements: a row for each element a RADBC (card RadiationToSpace: ambientNode "
            "NODAMB, viewFactor FAMB, controlNode CNTRLND) or a RADLST (card RadiationCavityList: cavity ICAVITY) "
            "names, a row for the ambient element of a RADCAV (card RadiationCavity: elementId ELEAMB, cavity), and a "
            "row "
            "for each cavity of a RADSET (card RadiationCavitySet: no elementId). "
            "radiationMatId is the RADM of the front face of the surface element (its back face when the front has "
            "none), "
            "absorptivity and emissivity the ABSORP and the first EMIS of that RADM; a value that does not apply is "
            "null. "
            "An element that does not exist is not listed. A negative EID of a RADBC (the back face) is listed by its "
            "ID.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "keep these surface elements")};
        info.defaultSort = "elementId (the cards without one last), then RADBC, RADLST, RADCAV";
        ColumnListBuilder columns;
        columns.int32("elementId").dictionary("card").int32("ambientNode").real("viewFactor").int32("controlNode");
        columns.int32("radiationMatId").reals("absorptivity emissivity").int32("cavity");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addRadiationTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<RadiationTableQuery>());
}

} // namespace oor::query
