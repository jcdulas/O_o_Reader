#include "oor/analysis/geometry/AeroPanels.hpp"

#include "oor/analysis/coordinates/CoordinateErrors.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsTable.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"
#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsTable.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace oor::analysis
{

namespace
{

// What the boxes of a panel are made from, whichever card it comes from.
struct PanelDefinition
{
    AeroPanelKind kind = AeroPanelKind::AeroLiftingPanel1;
    base::EntityId eid = 0;
    base::EntityId positionFrameId = 0;
    std::int32_t spanCount = 0; // NSPAN, 0 when the AEFACT lists the divisions
    base::EntityId spanList = 0;
    std::int32_t chordCount = 0; // NCHORD for a CAERO1; 1 for a strip (one box along the whole chord)
    base::EntityId chordList = 0;
    std::array<double, 3> point1 = {};
    double rootChord = 0.0;
    std::array<double, 3> point4 = {};
    double tipChord = 0.0;
};

// The panel as a message names it: the keyword of its card and its ID ("CAERO1 1000").
std::string describe(const model::Model&, const PanelDefinition& panel)
{
    return std::string(aeroPanelKindName(panel.kind)) + " " + std::to_string(panel.eid);
}

// The ID of the aerodynamic coordinate system: the AERO card's, else the AEROS card's, else the basic system.
base::EntityId aerodynamicSystemOf(const model::Model& model)
{
    if (const auto* aero = model.findTable<model::AeroUnsteadyParamsTable>())
    {
        for (std::uint32_t row = 0; row < aero->rows().rowCount(); ++row)
        {
            if (aero->rows().isAlive(row))
                return aero->card(row).aeroFrameId;
        }
    }
    if (const auto* aeros = model.findTable<model::AeroStaticParamsTable>())
    {
        for (std::uint32_t row = 0; row < aeros->rows().rowCount(); ++row)
        {
            if (aeros->rows().isAlive(row))
                return aeros->card(row).aeroFrameId;
        }
    }
    return 0;
}

// The fractions of the span or the chord at which the panel is cut: `count` equal divisions, or the numbers of the
// AEFACT.
std::vector<double> divisionPoints(const model::Model& model, const PanelDefinition& panel, std::int32_t count,
                                   base::EntityId list, const char* what)
{
    std::vector<double> points;
    if (count > 0)
    {
        points.reserve(static_cast<std::size_t>(count) + 1);
        for (std::int32_t index = 0; index <= count; ++index)
            points.push_back(static_cast<double>(index) / count);
        return points;
    }
    const auto* table = model.findTable<model::AeroFactorListTable>();
    const std::optional<model::CardLocation> found =
        table != nullptr ? model.locateInType(model::typeIndexOf<model::AeroFactorListTable>(), list) : std::nullopt;
    if (!found)
        throw GeometryError(describe(model, panel) + ": the " + std::string("AeroFactorList") + " " +
                            std::to_string(list) + " of the " + what + " divisions does not exist");
    const std::span<const double> values = table->valuesAt(found->row);
    if (values.size() < 2)
        throw GeometryError(describe(model, panel) + ": the " + std::string("AeroFactorList") + " " +
                            std::to_string(list) + " of the " + what + " divisions needs at least two points");
    points.assign(values.begin(), values.end());
    return points;
}

base::Vec3 vectorOf(const std::array<double, 3>& values) noexcept
{
    return base::Vec3{values[0], values[1], values[2]};
}

AeroPanelBoxes makeBoxes(const model::Model& model, const PanelDefinition& panel, CoordinateTransform& transform)
{
    const std::vector<double> spanPoints = divisionPoints(model, panel, panel.spanCount, panel.spanList, "span");
    const std::vector<double> chordPoints =
        panel.kind == AeroPanelKind::AeroLiftingPanel1
            ? divisionPoints(model, panel, panel.chordCount, panel.chordList, "chord")
            : std::vector<double>{0.0, 1.0};

    // The four corners of the panel in the basic system: the leading edge points go through their system, the trailing
    // edge ones are behind them along the x axis of the aerodynamic system.
    const base::Vec3 point1 = transform.toBasic(panel.positionFrameId, vectorOf(panel.point1));
    const base::Vec3 point4 = transform.toBasic(panel.positionFrameId, vectorOf(panel.point4));
    const base::EntityId aerodynamicSystem = aerodynamicSystemOf(model);
    const base::Vec3 flowAxis =
        aerodynamicSystem == 0 ? base::Vec3{1.0, 0.0, 0.0} : transform.frame(aerodynamicSystem).axes.column(0);
    const base::Vec3 point2 = point1 + flowAxis * panel.rootChord;
    const base::Vec3 point3 = point4 + flowAxis * panel.tipChord;

    // The bilinear interpolation of the panel: `along` runs from the leading edge (0) to the trailing edge (1),
    // `across` from the inboard edge (0) to the outboard one (1).
    const auto pointAt = [&](double along, double across)
    {
        const base::Vec3 inboard = point1 * (1.0 - along) + point2 * along;
        const base::Vec3 outboard = point4 * (1.0 - along) + point3 * along;
        return inboard * (1.0 - across) + outboard * across;
    };

    AeroPanelBoxes boxes;
    boxes.eid = panel.eid;
    boxes.kind = panel.kind;
    boxes.chordBoxes = static_cast<std::int32_t>(chordPoints.size()) - 1;
    boxes.spanBoxes = static_cast<std::int32_t>(spanPoints.size()) - 1;
    const std::size_t count = static_cast<std::size_t>(boxes.chordBoxes) * static_cast<std::size_t>(boxes.spanBoxes);
    boxes.boxId.reserve(count);
    boxes.chordIndex.reserve(count);
    boxes.spanIndex.reserve(count);
    boxes.corner1.reserve(count);
    boxes.corner2.reserve(count);
    boxes.corner3.reserve(count);
    boxes.corner4.reserve(count);
    boxes.center.reserve(count);
    boxes.controlPoint.reserve(count);
    boxes.loadPoint.reserve(count);
    boxes.area.reserve(count);
    boxes.chord.reserve(count);
    boxes.normal.reserve(count);

    for (std::int32_t span = 0; span < boxes.spanBoxes; ++span)
    {
        const double inboardFraction = spanPoints[static_cast<std::size_t>(span)];
        const double outboardFraction = spanPoints[static_cast<std::size_t>(span) + 1];
        const double middle = 0.5 * (inboardFraction + outboardFraction);
        for (std::int32_t chord = 0; chord < boxes.chordBoxes; ++chord)
        {
            const double leading = chordPoints[static_cast<std::size_t>(chord)];
            const double trailing = chordPoints[static_cast<std::size_t>(chord) + 1];
            const base::Vec3 corner1 = pointAt(leading, inboardFraction);
            const base::Vec3 corner2 = pointAt(trailing, inboardFraction);
            const base::Vec3 corner3 = pointAt(trailing, outboardFraction);
            const base::Vec3 corner4 = pointAt(leading, outboardFraction);
            // The diagonals 1-3 and 2-4 give the area and the normal of the quadrilateral.
            const base::Vec3 diagonals = base::cross(corner3 - corner1, corner4 - corner2);
            const double doubleArea = base::norm(diagonals);

            boxes.boxId.push_back(panel.eid + span * boxes.chordBoxes + chord);
            boxes.chordIndex.push_back(chord);
            boxes.spanIndex.push_back(span);
            boxes.corner1.push_back(corner1);
            boxes.corner2.push_back(corner2);
            boxes.corner3.push_back(corner3);
            boxes.corner4.push_back(corner4);
            boxes.center.push_back((corner1 + corner2 + corner3 + corner4) * 0.25);
            boxes.controlPoint.push_back(pointAt(leading + 0.75 * (trailing - leading), middle));
            boxes.loadPoint.push_back(pointAt(leading + 0.25 * (trailing - leading), middle));
            boxes.area.push_back(0.5 * doubleArea);
            boxes.chord.push_back(base::norm(pointAt(trailing, middle) - pointAt(leading, middle)));
            boxes.normal.push_back(doubleArea > 0.0 ? diagonals / doubleArea : base::Vec3{});
        }
    }
    return boxes;
}

// The panels of the model as definitions, by EID.
std::vector<PanelDefinition> panelDefinitions(const model::Model& model)
{
    std::vector<PanelDefinition> panels;
    if (const auto* table = model.findTable<model::AeroLiftingPanel1Table>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::AeroLiftingPanel1Card card = table->card(row);
            panels.push_back(PanelDefinition{AeroPanelKind::AeroLiftingPanel1, card.id, card.positionFrameId,
                                             card.spanBoxCount, card.spanDivisionListId, card.chordBoxCount,
                                             card.chordDivisionListId, card.point1, card.rootChord, card.point4,
                                             card.tipChord});
        }
    }
    if (const auto* table = model.findTable<model::AeroStripPanel4Table>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::AeroStripPanel4Card card = table->card(row);
            panels.push_back(PanelDefinition{AeroPanelKind::AeroStripPanel4, card.id, card.positionFrameId,
                                             card.spanBoxCount, card.spanDivisionListId, 1, 0, card.point1,
                                             card.rootChord, card.point4, card.tipChord});
        }
    }
    if (const auto* table = model.findTable<model::AeroPistonPanel5Table>())
    {
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::AeroPistonPanel5Card card = table->card(row);
            panels.push_back(PanelDefinition{AeroPanelKind::AeroPistonPanel5, card.id, card.positionFrameId,
                                             card.spanBoxCount, card.spanDivisionListId, 1, 0, card.point1,
                                             card.rootChord, card.point4, card.tipChord});
        }
    }
    std::ranges::sort(panels, {}, &PanelDefinition::eid);
    return panels;
}

} // namespace

std::string_view aeroPanelKindName(AeroPanelKind kind) noexcept
{
    switch (kind)
    {
    case AeroPanelKind::AeroLiftingPanel1:
        return "AeroLiftingPanel1";
    case AeroPanelKind::AeroStripPanel4:
        return "AeroStripPanel4";
    case AeroPanelKind::AeroPistonPanel5:
        return "AeroPistonPanel5";
    }
    return "AeroPanel";
}

AeroPanelBoxes aeroPanelBoxes(const model::Model& model, base::EntityId eid)
{
    CoordinateTransform transform(model);
    for (const PanelDefinition& panel : panelDefinitions(model))
    {
        if (panel.eid == eid)
            return makeBoxes(model, panel, transform);
    }
    throw GeometryError("no " + std::string("AeroLiftingPanel1") + ", " + std::string("AeroStripPanel4") + " or " +
                        std::string("AeroPistonPanel5") + " has the ID " + std::to_string(eid));
}

struct PanelFrame::Implementation
{
    explicit Implementation(const model::Model& model) : transform(model), aerodynamicSystem(aerodynamicSystemOf(model))
    {
    }

    CoordinateTransform transform;
    base::EntityId aerodynamicSystem;
};

PanelFrame::PanelFrame(const model::Model& model) : m_implementation(std::make_unique<Implementation>(model)) {}

PanelFrame::~PanelFrame() = default;

base::Vec3 PanelFrame::toBasic(base::EntityId cp, const base::Vec3& point)
{
    return m_implementation->transform.toBasic(cp, point);
}

base::Vec3 PanelFrame::flowDirection()
{
    const base::EntityId system = m_implementation->aerodynamicSystem;
    return system == 0 ? base::Vec3{1.0, 0.0, 0.0} : m_implementation->transform.frame(system).axes.column(0);
}

double PanelFrame::spanBetween(const base::Vec3& inboard, const base::Vec3& outboard)
{
    const base::Vec3 flow = flowDirection();
    const base::Vec3 along = outboard - inboard;
    return base::norm(along - flow * base::dot(along, flow));
}

std::vector<AeroPanelBoxes> allAeroPanelBoxes(const model::Model& model, std::vector<base::EntityId>* skipped)
{
    CoordinateTransform transform(model);
    std::vector<AeroPanelBoxes> result;
    for (const PanelDefinition& panel : panelDefinitions(model))
    {
        try
        {
            result.push_back(makeBoxes(model, panel, transform));
        }
        catch (const base::OorError&)
        {
            if (skipped != nullptr)
                skipped->push_back(panel.eid);
        }
    }
    return result;
}

} // namespace oor::analysis
