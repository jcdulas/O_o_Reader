#include "ElementProvider.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"

#include <algorithm>
#include <numbers>

namespace oor::analysis::detail
{

base::Mat3 GeometryContext::displacementRotation(std::uint32_t gridRow) const
{
    const model::NodeTable* grids = m_model.findTable<model::NodeTable>();
    const base::EntityId cd = grids->displacementFrameId(gridRow);
    if (cd <= 0)
        return base::Mat3::identity();
    return systemAxesAt(cd, m_cache.position(gridRow));
}

const CoordinateFrame& GeometryContext::frameOf(base::EntityId cid) const
{
    auto known = m_frames.find(cid);
    if (known == m_frames.end())
    {
        // The cache resolves systems on demand; it is shared by every context, so one at a time.
        const std::lock_guard<std::mutex> lock(m_cache.mutex());
        if (!m_cache.transform().hasSystem(cid))
            throw CoordinateSystemError("coordinate system " + std::to_string(cid) + " does not exist");
        known = m_frames.emplace(cid, m_cache.transform().frame(cid)).first;
    }
    return known->second;
}

base::Mat3 GeometryContext::systemAxesAt(base::EntityId cid, const base::Vec3& basicPoint) const
{
    if (cid == 0)
        return base::Mat3::identity();
    return frameOf(cid).directionsAt(basicPoint);
}

base::Vec3 GeometryContext::pointInSystem(base::EntityId cid, const base::Vec3& coordinates) const
{
    if (cid == 0)
        return coordinates;
    return frameOf(cid).toBasic(coordinates);
}

namespace
{

// The thickness a PSHELL1 gives its elements: the mean of the thicknesses at the grid points that it states. T1 blank
// or zero leaves the thickness to the element card, which the property cannot know: no thickness. A T2
// to T4 that is zero is a thickness the deck left blank, so it does not count.
double meanThickness(const model::ShellPropNodalCard& card)
{
    if (card.thickness1 <= 0.0)
        return NOT_DEFINED;
    double total = 0.0;
    int stated = 0;
    for (const double thickness : {card.thickness1, card.thickness2, card.thickness3, card.thickness4})
    {
        if (thickness > 0.0)
        {
            total += thickness;
            ++stated;
        }
    }
    return total / stated;
}

} // namespace

double GeometryContext::shellThickness(base::EntityId pid) const
{
    if (const auto known = m_thickness.find(pid); known != m_thickness.end())
        return known->second;
    double thickness = NOT_DEFINED;
    if (const auto location = m_model.locate(base::IdSpace::Property, pid))
    {
        const std::uint16_t type = location->table;
        const model::CardTable& table = *m_model.findTable(type);
        if (type == model::typeIndexOf<model::ShellPropTable>())
            thickness = static_cast<const model::ShellPropTable&>(table).card(location->row).thickness;
        else if (type == model::typeIndexOf<model::CompPropTable>())
            thickness = static_cast<const model::CompPropTable&>(table).card(location->row).totalThickness();
        else if (type == model::typeIndexOf<model::CompPropGlobalTable>())
            thickness = static_cast<const model::CompPropGlobalTable&>(table).card(location->row).totalThickness();
        else if (type == model::typeIndexOf<model::ShearPanelPropTable>())
            thickness = static_cast<const model::ShearPanelPropTable&>(table).card(location->row).thickness;
        else if (type == model::typeIndexOf<model::ShellPropNodalTable>())
            thickness = meanThickness(static_cast<const model::ShellPropNodalTable&>(table).card(location->row));
    }
    m_thickness.emplace(pid, thickness);
    return thickness;
}

bool GeometryContext::isLaminate(base::EntityId pid) const
{
    const auto location = m_model.locate(base::IdSpace::Property, pid);
    return location && (location->table == model::typeIndexOf<model::CompPropTable>() ||
                        location->table == model::typeIndexOf<model::CompPropGlobalTable>());
}

double GeometryContext::sectionArea(base::EntityId pid) const
{
    if (const auto known = m_area.find(pid); known != m_area.end())
        return known->second;
    double area = NOT_DEFINED;
    if (const auto location = m_model.locate(base::IdSpace::Property, pid))
    {
        const std::uint16_t type = location->table;
        const model::CardTable& table = *m_model.findTable(type);
        if (type == model::typeIndexOf<model::RodPropTable>())
        {
            area = static_cast<const model::RodPropTable&>(table).card(location->row).area;
        }
        else if (type == model::typeIndexOf<model::TubePropTable>())
        {
            // The ring between the outer diameter and the inner one (OD - 2 T); a zero T is a solid rod.
            const model::TubePropCard card = static_cast<const model::TubePropTable&>(table).card(location->row);
            const double inner = card.thickness > 0.0 ? std::max(0.0, card.outerDiameter - 2.0 * card.thickness) : 0.0;
            area = std::numbers::pi / 4.0 * (card.outerDiameter * card.outerDiameter - inner * inner);
        }
        else if (type == model::typeIndexOf<model::BarPropTable>())
        {
            area = static_cast<const model::BarPropTable&>(table).card(location->row).area;
        }
        else if (SectionFormulas::supports(type))
        {
            // PBEAM, PBEAML, PBEAM3, PBCOMP and PBARL: the section of the property, averaged along the beam.
            if (const auto section = SectionFormulas::of(m_model, *location, ShearFactorRule::None))
                area = section->meanArea();
        }
    }
    m_area.emplace(pid, area);
    return area;
}

} // namespace oor::analysis::detail
