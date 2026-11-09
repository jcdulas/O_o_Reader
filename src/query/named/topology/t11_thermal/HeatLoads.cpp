#include "oor/query/named/topology/t11_thermal/HeatLoads.hpp"

#include "ThermalQuerySupport.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/analysis/geometry/SurfaceSides.hpp"
#include "oor/model/cards/thermal/heatFluxBoundaryLoad3/HeatFluxBoundaryLoad3Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodalLoad2/HeatFluxNodalLoad2Table.hpp"
#include "oor/model/cards/thermal/heatFluxNodeSetLoad/HeatFluxNodeSetLoadTable.hpp"
#include "oor/model/cards/thermal/heatFluxSurfaceLoad1/HeatFluxSurfaceLoad1Table.hpp"
#include "oor/model/cards/thermal/heatFluxVectorLoad/HeatFluxVectorLoadTable.hpp"
#include "oor/model/cards/thermal/heatVolumeSourceLoad/HeatVolumeSourceLoadTable.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/common/ThermalMaterials.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <numbers>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The area a QHBDY covers (format reference, QHBDY: the flux is applied to the points through the area of the face): AF
// for a POINT, AF times the length for a LINE, the surface of the cone for a REV, the area of the polygon of the
// corners for an AREA.
double qhbdyArea(const model::HeatFluxNodeSetLoadCard& card, const NodePositions& positions)
{
    const auto at = [&](std::size_t index) { return positions.position(card.nodes[index]); };
    const auto has = [&](std::size_t count)
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            if (card.nodes[index] == 0 || !positions.has(card.nodes[index]))
                return false;
        }
        return true;
    };
    if (card.faceKind == "Point")
        return card.area.value_or(NOT_A_NUMBER);
    if (card.faceKind == "Line")
        return has(2) && card.area ? *card.area * base::norm(at(1) - at(0)) : NOT_A_NUMBER;
    if (card.faceKind == "ReversedNormal")
        return has(2) ? std::numbers::pi * (std::fabs(at(0).x) + std::fabs(at(1).x)) * base::norm(at(1) - at(0))
                      : NOT_A_NUMBER;
    if (card.faceKind == "AreaTria3" || card.faceKind == "AreaTria6")
        return has(3) ? 0.5 * base::norm(base::cross(at(1) - at(0), at(2) - at(0))) : NOT_A_NUMBER;
    if (card.faceKind == "AreaQuad4" || card.faceKind == "AreaQuad8")
        return has(4) ? 0.5 * base::norm(base::cross(at(2) - at(0), at(3) - at(1))) : NOT_A_NUMBER;
    return NOT_A_NUMBER;
}

// The unit vector of the flux in the basic system: the components E in the system CE at the centre of the face (the
// direction of a cylindrical or spherical system changes from point to point). NaN when the system does
// not exist.
base::Vec3 flowDirection(analysis::CoordinateTransform& transform, base::EntityId ce, const base::Vec3& components,
                         const base::Vec3& center)
{
    base::Vec3 direction = components;
    if (ce > 0)
    {
        try
        {
            direction = transform.frame(ce).directionsAt(center) * components;
        }
        catch (const std::exception&)
        {
            return base::Vec3{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
        }
    }
    return base::normalized(direction);
}

// The QVOL cards: the heat generated in each element is the rate times the multiplier HGEN of the material times the
// volume.
void addVolumeLoads(const std::shared_ptr<model::Model>& modelPointer, ExecutionContext& context,
                    const model::HeatVolumeSourceLoadTable& table, const std::function<bool(base::EntityId)>& selected,
                    std::vector<HeatLoad>& loads)
{
    const model::Model& model = *modelPointer;
    const std::vector<std::uint16_t> types = typesInSpace(model::CardCategory::Element, base::IdSpace::Element);
    const CardList everyElement = listCards(model, types);
    // The elements the cards name: a range takes the elements of the model in it.
    std::vector<std::int64_t> chosen;
    for (std::uint32_t at = 0; at < table.rows().rowCount(); ++at)
    {
        if (!table.rows().isAlive(at) || !selected(table.idAt(at)))
            continue;
        for (const base::EntityId eid : thermal::existingIdsOfRuns(table.card(at).elements, everyElement.ids))
            chosen.push_back(eid);
    }
    std::sort(chosen.begin(), chosen.end());
    chosen.erase(std::unique(chosen.begin(), chosen.end()), chosen.end());
    if (chosen.empty())
        return;
    const CardList elements = listCards(model, types, chosen);
    ListColumns columns(modelPointer, elements, context);
    const std::vector<double> volumes = columns.reals("volume");
    CardFacts facts(model);
    std::map<base::EntityId, std::size_t> indexOf;
    for (std::size_t index = 0; index < elements.size(); ++index)
        indexOf.emplace(elements.ids[index], index);
    for (std::uint32_t at = 0; at < table.rows().rowCount(); ++at)
    {
        if (!table.rows().isAlive(at) || !selected(table.idAt(at)))
            continue;
        const model::HeatVolumeSourceLoadCard card = table.card(at);
        for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, everyElement.ids))
        {
            const std::size_t index = indexOf.at(eid);
            HeatLoad row;
            row.sid = card.id;
            row.card = "HeatVolumeSourceLoad";
            row.targetType = "element";
            row.target = eid;
            row.flux = card.heatGeneration;
            row.extent = volumes[index];
            row.control = card.controlNode;
            row.power =
                card.heatGeneration * heatGenerationMultiplier(facts, model, elements.location(index)) * row.extent;
            loads.push_back(row);
        }
    }
}

} // namespace

std::vector<HeatLoad> computeHeatLoads(const std::shared_ptr<model::Model>& modelPointer, ExecutionContext& context,
                                       const std::optional<std::vector<base::EntityId>>& sets)
{
    const model::Model& model = *modelPointer;
    const auto selected = [&](base::EntityId sid)
    { return !sets || std::binary_search(sets->begin(), sets->end(), sid); };
    const thermal::SurfaceElements surfaces(model, context.pool());

    std::vector<HeatLoad> loads;
    const auto surfaceRow =
        [&](base::EntityId sid, const char* card, base::EntityId eid, double flux, base::EntityId control)
    {
        HeatLoad row;
        row.sid = sid;
        row.card = card;
        row.targetType = "surfaceElement";
        row.target = eid;
        row.flux = flux;
        row.control = control;
        if (const thermal::SurfaceElement* surface = surfaces.find(eid))
        {
            row.extent = surface->area;
            row.power = flux * surface->area;
        }
        return row;
    };
    if (const auto* table = model.findTable<model::HeatFluxSurfaceLoad1Table>())
    {
        for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
        {
            if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                continue;
            const model::HeatFluxSurfaceLoad1Card card = table->card(at);
            for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, surfaces.ids()))
                loads.push_back(surfaceRow(card.id, "HeatFluxSurfaceLoad1", eid, card.heatFlux, 0));
        }
    }
    if (const auto* table = model.findTable<model::HeatFluxNodalLoad2Table>())
    {
        for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
        {
            if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                continue;
            const model::HeatFluxNodalLoad2Card card = table->card(at);
            HeatLoad row = surfaceRow(card.id, "HeatFluxNodalLoad2", card.elemId, NOT_A_NUMBER, 0);
            // The flux at each corner point: the surface takes it as the mean (the area of each point is the same, so
            // the power is the area times the mean; exact for a flux that does not vary over the face).
            const auto location = model.locate(base::IdSpace::SurfaceElement, card.elemId);
            if (location)
            {
                const analysis::SurfaceDescription description = analysis::describeSurface(model, *location);
                const std::size_t corners = std::min(description.corners, card.heatFlux.size());
                if (corners > 0)
                {
                    double sum = 0.0;
                    for (std::size_t index = 0; index < corners; ++index)
                        sum += card.heatFlux[index];
                    row.flux = sum / static_cast<double>(corners);
                    row.power = row.flux * row.extent;
                }
            }
            loads.push_back(row);
        }
    }
    if (const auto* table = model.findTable<model::HeatFluxBoundaryLoad3Table>())
    {
        for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
        {
            if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                continue;
            const model::HeatFluxBoundaryLoad3Card card = table->card(at);
            for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, surfaces.ids()))
                loads.push_back(surfaceRow(card.id, "HeatFluxBoundaryLoad3", eid, card.heatFlux, card.controlNode));
        }
    }
    if (const auto* table = model.findTable<model::HeatFluxNodeSetLoadTable>())
    {
        std::optional<NodePositions> positions;
        for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
        {
            if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                continue;
            if (!positions)
                positions.emplace(model, context.pool());
            const model::HeatFluxNodeSetLoadCard card = table->card(at);
            HeatLoad row;
            row.sid = card.id;
            row.card = "HeatFluxNodeSetLoad";
            row.targetType = "node";
            row.target = card.nodes[0];
            row.targetValid = card.nodes[0] != 0;
            row.flux = card.heatFlux;
            row.extent = qhbdyArea(card, *positions);
            row.power = row.flux * row.extent;
            loads.push_back(row);
        }
    }
    if (const auto* table = model.findTable<model::HeatVolumeSourceLoadTable>())
    {
        addVolumeLoads(modelPointer, context, *table, selected, loads);
    }
    if (const auto* table = model.findTable<model::HeatFluxVectorLoadTable>())
    {
        analysis::CoordinateTransform transform(model);
        for (std::uint32_t at = 0; at < table->rows().rowCount(); ++at)
        {
            if (!table->rows().isAlive(at) || !selected(table->idAt(at)))
                continue;
            const model::HeatFluxVectorLoadCard card = table->card(at);
            for (const base::EntityId eid : thermal::existingIdsOfRuns(card.elements, surfaces.ids()))
            {
                HeatLoad row = surfaceRow(card.id, "HeatFluxVectorLoad", eid, card.heatFlux.value_or(NOT_A_NUMBER),
                                          card.controlNode);
                const thermal::SurfaceElement* surface = surfaces.find(eid);
                const bool timeDependent = card.tableId[0] != 0 || card.tableId[1] != 0 || card.tableId[2] != 0;
                const base::Vec3 vector{card.direction[0], card.direction[1], card.direction[2]};
                if (!timeDependent && base::norm(vector) > 0.0 && surface != nullptr)
                {
                    row.direction =
                        flowDirection(transform, card.directionFrameId.value_or(0), vector, surface->center);
                    // The power into a face is the flux times the area the source sees: the cosine of the angle
                    // between the flux vector and the normal of the face (either side of the face counts).
                    row.power = row.flux * surface->area * std::fabs(base::dot(row.direction, surface->normal));
                }
                else
                {
                    row.power = NOT_A_NUMBER;
                }
                loads.push_back(row);
            }
        }
    }
    std::stable_sort(loads.begin(), loads.end(),
                     [](const HeatLoad& left, const HeatLoad& right) { return left.sid < right.sid; });
    return loads;
}

} // namespace oor::query
