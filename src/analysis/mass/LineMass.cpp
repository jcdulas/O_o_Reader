#include "MassProvider.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MassProperties.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/beam3/Beam3Table.hpp"
#include "oor/model/cards/elements/curvedBeam2/CurvedBeam2Table.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"

#include <array>
#include <cmath>

// The mass of the rods, tubes, bars and beams: (density times the area + NSM) times the length, the length between the
// offset ends of a bar or beam. A bar or beam whose section varies (PBEAM, PBEAML, PBEAM3 with stations) has the
// integral of that along the beam, the section varying linearly between the stations (the trapezoid over X/XB). The
// mass of a CBEND needs the length of the arc, which the geometry does not compute: it is NaN.
//
// The structural mass is at the centroid of its distribution along the beam (the middle of the beam for a uniform one).
// The non-structural mass of a PBEAM, PBEAM3 or PBCOMP is at its own centre of gravity, which the property gives as
// (y, z) in the element system at the two ends: the element has then the two masses as one body at their common centre
// with the inertia that comes from their distance.
//
// The offsets (WA, WB; WC of a CBEAM3) place that mass by MDLPRM OFFDEF (MassCalculator::offsetMassOf): between the
// offset ends under LROFF and ELMZ; under ELMOFF, the solver's default, at the grid points (format reference CBAR
// the masses are transformed for the offsets under LROFF), the point at a fraction of the length between the
// offset ends at the same fraction between the grid points, the non-structural mass off the axis kept in the element
// axes. The amount is the same: the mass of the length between the offset ends.

namespace oor::analysis::detail
{

namespace
{

constexpr std::uint8_t LINE_QUANTITIES = bit(Quantity::Center) | bit(Quantity::Length);
constexpr std::uint8_t BEAM_QUANTITIES = LINE_QUANTITIES | bit(Quantity::Frame);

// The mass of a line element with a property that gives its section by stations.
//
// `shiftAlong` lets the structural mass sit away from the middle of the beam when the section varies; the element frame
// is needed then (and for an off-axis non-structural mass). `gridEnds`, when not null, are the grid points GA and GB:
// the mass is between them instead of between the offset ends.
void beamMass(const BeamSection& section, double density, const ElementValues& geometry, bool shiftAlong,
              const std::array<base::Vec3, 2>* gridEnds, MassValue& value)
{
    std::vector<double> positions;
    std::vector<double> areas;
    std::vector<double> nsms;
    for (const SectionProperties& station : section.stations)
    {
        positions.push_back(station.xxb);
        areas.push_back(station.area);
        nsms.push_back(station.nsm);
    }
    const LinearIntegral area = integrateAlongBeam(positions, areas);
    const LinearIntegral nsm = integrateAlongBeam(positions, nsms);
    const double length = geometry.length;
    value.structural = density * area.integral * length;
    value.nonStructural = nsm.integral * length;
    value.center = geometry.center;

    const bool frameKnown = isDefined(geometry.frame.column(0));
    const base::Vec3 axis = frameKnown ? geometry.frame.column(0) : undefinedVector();
    const auto along = [&](double fraction)
    {
        if (gridEnds != nullptr)
        {
            const double at = shiftAlong ? fraction : 0.5;
            return (*gridEnds)[0] + at * ((*gridEnds)[1] - (*gridEnds)[0]);
        }
        if (!shiftAlong || !frameKnown || std::abs(fraction - 0.5) < 1.0e-12)
            return geometry.center;
        return geometry.center + (fraction - 0.5) * length * axis;
    };
    const base::Vec3 structuralCenter = along(area.centroid);
    base::Vec3 nonStructuralCenter = along(nsm.centroid);
    const double ey = section.nsmCenterA[0] + nsm.centroid * (section.nsmCenterB[0] - section.nsmCenterA[0]);
    const double ez = section.nsmCenterA[1] + nsm.centroid * (section.nsmCenterB[1] - section.nsmCenterA[1]);
    if ((ey != 0.0 || ez != 0.0) && isDefined(geometry.frame.column(1)))
        nonStructuralCenter = nonStructuralCenter + ey * geometry.frame.column(1) + ez * geometry.frame.column(2);

    const double total = value.structural + value.nonStructural;
    if (!(total != 0.0) || !isDefined(structuralCenter) || !isDefined(nonStructuralCenter))
    {
        value.center = structuralCenter;
        return;
    }
    value.center = (value.structural * structuralCenter + value.nonStructural * nonStructuralCenter) / total;
    if (value.structural != 0.0 && value.nonStructural != 0.0 &&
        (structuralCenter.x != nonStructuralCenter.x || structuralCenter.y != nonStructuralCenter.y ||
         structuralCenter.z != nonStructuralCenter.z))
    {
        const base::Mat3 first = pointMassInertia(value.structural, structuralCenter - value.center);
        const base::Mat3 second = pointMassInertia(value.nonStructural, nonStructuralCenter - value.center);
        for (std::size_t index = 0; index < 9; ++index)
            value.inertia.values[index] = first.values[index] + second.values[index];
        value.hasInertia = true;
    }
}

// True when an offset vector of the card is not zero.
template <typename... Offsets>
bool hasOffsets(const Offsets&... offsets)
{
    return ((offsets[0] != 0.0 || offsets[1] != 0.0 || offsets[2] != 0.0) || ...);
}

// Rod2 and Tube2: a property gives the section.
template <typename TableT>
struct RodShape
{
    static constexpr std::uint8_t QUANTITIES = LINE_QUANTITIES;

    static void evaluate(const TableT& table, std::uint32_t row, MassContext& context, const ElementValues& geometry,
                         MassValue& value)
    {
        const base::EntityId pid = table.card(row).propId;
        const MassContext::LineData& data = context.line(pid, Owner{TableT::NAME.data(), table.idAtRow(row)});
        const SectionProperties& station = data.section.stations.front();
        value.property = pid;
        value.material = data.section.materialId;
        value.center = geometry.center;
        value.structural = data.density * station.area * geometry.length;
        value.nonStructural = station.nsm * geometry.length;
    }
};

// Rod2InlineSection: the section is on the element.
struct Rod2InlineSectionShape
{
    static constexpr std::uint8_t QUANTITIES = LINE_QUANTITIES;

    static void evaluate(const model::Rod2InlineSectionTable& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::Rod2InlineSectionCard card = table.card(row);
        value.material = card.matId;
        value.center = geometry.center;
        value.structural = context.density(card.matId, Owner{model::Rod2InlineSectionTable::NAME.data(), card.id}) *
                           card.area * geometry.length;
        value.nonStructural = card.nonStructMass * geometry.length;
    }
};

// Bar2: PBAR or PBARL, one station.
struct BarShape
{
    static constexpr std::uint8_t QUANTITIES = BEAM_QUANTITIES;

    static void evaluate(const model::Bar2Table& table, std::uint32_t row, MassContext& context,
                         const ElementValues& geometry, MassValue& value)
    {
        const model::Bar2Card card = table.card(row);
        const MassContext::LineData& data = context.line(card.propId, Owner{model::Bar2Table::NAME.data(), card.id});
        value.property = card.propId;
        value.material = data.section.materialId;
        const bool atGrids = !context.massOnOffsets() && hasOffsets(card.offsetA, card.offsetB);
        const std::array<base::Vec3, 2> grids =
            atGrids ? std::array<base::Vec3, 2>{context.gridPosition(card.nodeA), context.gridPosition(card.nodeB)}
                    : std::array<base::Vec3, 2>{};
        beamMass(data.section, data.density, geometry, true, atGrids ? &grids : nullptr, value);
    }
};

// Beam2 and Beam3: PBEAM, PBEAML, PBCOMP, PBEAM3.
template <typename TableT>
struct BeamShape
{
    static constexpr std::uint8_t QUANTITIES = BEAM_QUANTITIES;

    static void evaluate(const TableT& table, std::uint32_t row, MassContext& context, const ElementValues& geometry,
                         MassValue& value)
    {
        const auto card = table.card(row);
        const MassContext::LineData& data = context.line(card.propId, Owner{TableT::NAME.data(), table.idAtRow(row)});
        value.property = card.propId;
        value.material = data.section.materialId;
        if constexpr (std::is_same_v<TableT, model::Beam2Table>)
        {
            const bool atGrids = !context.massOnOffsets() && hasOffsets(card.offsetA, card.offsetB);
            const std::array<base::Vec3, 2> grids =
                atGrids ? std::array<base::Vec3, 2>{context.gridPosition(card.nodeA), context.gridPosition(card.nodeB)}
                        : std::array<base::Vec3, 2>{};
            beamMass(data.section, data.density, geometry, true, atGrids ? &grids : nullptr, value);
        }
        else
        {
            // The arc of a CBEAM3 is not straight: its mass stays at the middle node, GC offset by WC (or the middle
            // of the offset ends), or GC itself (the middle of GA and GB) when the mass is at the grid points.
            beamMass(data.section, data.density, geometry, false, nullptr, value);
            if constexpr (std::is_same_v<TableT, model::Beam3Table>)
            {
                if (!context.massOnOffsets() && hasOffsets(card.offsetA, card.offsetB, card.offsetC) &&
                    isDefined(value.center))
                {
                    const base::Vec3 grid =
                        card.nodeC != 0 ? context.gridPosition(card.nodeC)
                                        : 0.5 * (context.gridPosition(card.nodeA) + context.gridPosition(card.nodeB));
                    value.center = value.center + (grid - geometry.center);
                }
            }
        }
    }
};

} // namespace

void registerLineMass(MassRegistry& registry)
{
    registry.add<model::Rod2Table, RodShape<model::Rod2Table>>();
    registry.add<model::Tube2Table, RodShape<model::Tube2Table>>();
    registry.add<model::Rod2InlineSectionTable, Rod2InlineSectionShape>();
    registry.add<model::Bar2Table, BarShape>();
    registry.add<model::Beam2Table, BeamShape<model::Beam2Table>>();
    registry.add<model::Beam3Table, BeamShape<model::Beam3Table>>();
    registry.add<model::CurvedBeam2Table, BeamShape<model::CurvedBeam2Table>>();
}

} // namespace oor::analysis::detail
