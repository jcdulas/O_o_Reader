#include "ElementProvider.hpp"
#include "ShellGeometry.hpp"
#include "SolidVolume.hpp"
#include "oor/model/cards/thermal/acousticAbsorberHexa/AcousticAbsorberHexaTable.hpp"
#include "oor/model/cards/thermal/acousticAbsorber/AcousticAbsorberTable.hpp"
#include "oor/model/cards/thermal/acousticBarrierHexa/AcousticBarrierHexaTable.hpp"

#include <array>

// The acoustic elements of the fluid-structure models.
//
// CAABSF is a surface (an absorber on the skin of the fluid): three or four corners, G4 blank for a triangle. It has
// the area, the normal and the centre of a shell with no thickness and no element system.
//
// CHACAB (the fluid cavity element) and CHACBR (the acoustic barrier) are hexahedra with the nodes G1 to G12 and G17 to
// G20: the fields of the edge nodes between the two faces (G13 to G16 of a CHEXA) do not exist. The volume is the
// isoparametric one of the hexahedron (an edge node that is missing is at the middle of its edge) and the centre is the
// average of the eight corners.

namespace oor::analysis::detail
{

namespace
{

struct AcousticAbsorberShape
{
    static void nodes(const model::AcousticAbsorberTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::AcousticAbsorberTable& table, const Evaluation& evaluation,
                         ElementValues& values)
    {
        const model::AcousticAbsorberCard card = table.card(evaluation.row);
        ShellInput input;
        input.pid = card.propId;
        input.hasThickness = false;
        input.hasFrame = false;
        if (card.nodes[3] == 0)
        {
            input.corners = evaluation.points.first(3);
            evaluateTriangle(evaluation, input, values);
        }
        else
        {
            input.corners = evaluation.points.first(4);
            evaluateQuadrilateral(evaluation, input, values);
        }
    }
};

// The nodes of a CHACAB or CHACBR in the order of a CHEXA: G1 to G12, the four edge nodes between the faces missing,
// then G17 to G20 (the 16 slots of the card are those that exist).
void evaluateAcousticHexahedron(const Evaluation& evaluation, ElementValues& values)
{
    const std::span<const base::Vec3> p = evaluation.points;
    for (std::size_t corner = 0; corner < 8; ++corner)
    {
        if (!isFinite(p[corner]))
            return;
    }
    if (evaluation.wants(Quantity::Center))
        values.center = (p[0] + p[1] + p[2] + p[3] + p[4] + p[5] + p[6] + p[7]) / 8.0;
    if (!evaluation.wants(Quantity::Volume))
        return;
    std::array<base::Vec3, 20> hexahedron;
    hexahedron.fill(undefinedVector());
    for (std::size_t slot = 0; slot < 12; ++slot)
        hexahedron[slot] = p[slot];
    for (std::size_t slot = 12; slot < 16; ++slot)
        hexahedron[slot + 4] = p[slot];
    bool anyEdge = false;
    for (std::size_t slot = 8; slot < 20; ++slot)
        anyEdge = anyEdge || isFinite(hexahedron[slot]);
    values.volume =
        anyEdge ? hexahedronVolume(hexahedron) : hexahedronVolume(std::span<const base::Vec3>(hexahedron).first(8));
}

struct AcousticAbsorberHexaShape
{
    static void nodes(const model::AcousticAbsorberHexaTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::AcousticAbsorberHexaTable&, const Evaluation& evaluation,
                         ElementValues& values)
    {
        evaluateAcousticHexahedron(evaluation, values);
    }
};

struct AcousticBarrierHexaShape
{
    static void nodes(const model::AcousticBarrierHexaTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        const auto card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::AcousticBarrierHexaTable&, const Evaluation& evaluation,
                         ElementValues& values)
    {
        evaluateAcousticHexahedron(evaluation, values);
    }
};

} // namespace

void registerAcousticProviders(ProviderRegistry& registry)
{
    registry.add<model::AcousticAbsorberTable, AcousticAbsorberShape>();
    registry.add<model::AcousticAbsorberHexaTable, AcousticAbsorberHexaShape>();
    registry.add<model::AcousticBarrierHexaTable, AcousticBarrierHexaShape>();
}

} // namespace oor::analysis::detail
