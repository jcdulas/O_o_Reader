#include "MassProvider.hpp"
#include "oor/analysis/coordinates/BasicPositionCache.hpp"
#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/mass/MassErrors.hpp"
#include "oor/analysis/mass/MaterialProperties.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/beamProp3/BeamProp3Table.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropTable.hpp"
#include "oor/model/cards/properties/fastenerProp/FastenerPropTable.hpp"
#include "oor/model/cards/properties/massProp/MassPropTable.hpp"
#include "oor/model/cards/properties/planePropNonlinear/PlanePropNonlinearTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/shearPanelProp/ShearPanelPropTable.hpp"
#include "oor/model/cards/properties/shellProp/ShellPropTable.hpp"
#include "oor/model/cards/properties/shellPropNodal/ShellPropNodalTable.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"
#include "oor/model/cards/properties/solidPropHyperelastic/SolidPropHyperelasticTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"

#include <string>

namespace oor::analysis::detail
{

namespace
{

template <typename TableT>
typename TableT::CardType cardAt(const model::Model& model, model::CardLocation location)
{
    return static_cast<const TableT&>(*model.findTable(location.table)).card(location.row);
}

// The density of a material that may be absent (MID 0 or negative: none); throws for a material that does not exist.
double densityOrThrow(const model::Model& model, base::EntityId mid, const Owner& owner)
{
    const std::optional<double> density = MaterialProperties::density(model, mid);
    if (!density)
    {
        throw MassError(owner.text(model) + " refers to material " + std::to_string(mid) +
                        ", which does not exist or has no mass density");
    }
    return *density;
}

} // namespace

base::Vec3 MassContext::gridPosition(base::EntityId nid) const
{
    const auto location = model().locate(base::IdSpace::Node, nid);
    if (!location || location->table != model::typeIndexOf<model::NodeTable>())
        return undefinedVector();
    return geometry().cache().position(location->row);
}

double MassContext::propertyDensity(base::EntityId mid, base::EntityId pid, const Owner& owner)
{
    try
    {
        return density(mid, Owner{"property", pid});
    }
    catch (const MassError& error)
    {
        throw MassError(owner.text(model()) + ": " + error.what());
    }
}

double MassContext::density(base::EntityId mid, const Owner& owner)
{
    if (const auto known = m_density.find(mid); known != m_density.end())
        return known->second;
    const double value = densityOrThrow(model(), mid, owner);
    m_density.emplace(mid, value);
    return value;
}

const ShellData& MassContext::shell(base::EntityId pid, const Owner& owner)
{
    if (const auto known = m_shell.find(pid); known != m_shell.end())
        return known->second;
    ShellData data;
    const auto location = model().locate(base::IdSpace::Property, pid);
    if (!location)
        throw MassError(owner.text(model()) + " refers to property " + std::to_string(pid) + ", which does not exist");
    const std::uint16_t type = location->table;
    if (type == model::typeIndexOf<model::ShellPropTable>())
    {
        const model::ShellPropCard card = cardAt<model::ShellPropTable>(model(), *location);
        // The material of the mass is MID1, or MID2 when there is no membrane material (the bending one).
        const base::EntityId mid =
            card.membraneMatId > 0 ? card.membraneMatId : (card.bendingMatId > 0 ? card.bendingMatId : 0);
        data.kind = ShellData::Kind::Plate;
        data.density = mid > 0 ? propertyDensity(mid, pid, owner) : 0.0;
        data.nsm = card.nonStructMass;
        data.material = mid;
    }
    else if (type == model::typeIndexOf<model::ShellPropNodalTable>())
    {
        // A SOL 700 shell: the density of its material and the thickness of the element (the mean of T1 to T4); no
        // non-structural mass.
        const model::ShellPropNodalCard card = cardAt<model::ShellPropNodalTable>(model(), *location);
        data.kind = ShellData::Kind::Plate;
        data.density = propertyDensity(card.matId, pid, owner);
        data.material = card.matId;
    }
    else if (type == model::typeIndexOf<model::CompPropTable>() ||
             type == model::typeIndexOf<model::CompPropGlobalTable>())
    {
        data.kind = ShellData::Kind::Laminate;
        double nsm = 0.0;
        nsm = type == model::typeIndexOf<model::CompPropTable>()
                  ? cardAt<model::CompPropTable>(model(), *location).nonStructMass
                  : cardAt<model::CompPropGlobalTable>(model(), *location).nonStructMass;
        // Z0 places the plies, not the mass (the solver: format reference MDLPRM OFFDEF note;
        // ShellMass.cpp)
        for (const LaminatePly& ply : Laminate::stack(model(), *location))
            data.massPerArea += propertyDensity(ply.materialId, pid, owner) * ply.thickness;
        data.nsm = nsm;
    }
    else if (type == model::typeIndexOf<model::ShearPanelPropTable>())
    {
        const model::ShearPanelPropCard card = cardAt<model::ShearPanelPropTable>(model(), *location);
        data.kind = ShellData::Kind::Plate;
        data.density = propertyDensity(card.matId, pid, owner);
        data.nsm = card.nonStructMass;
        data.material = card.matId;
    }
    else if (type == model::typeIndexOf<model::PlanePropNonlinearTable>())
    {
        const model::PlanePropNonlinearCard card = cardAt<model::PlanePropNonlinearTable>(model(), *location);
        data.kind = ShellData::Kind::Volume;
        data.density = propertyDensity(card.matId, pid, owner);
        data.material = card.matId;
    }
    return m_shell.emplace(pid, data).first->second;
}

const SolidData& MassContext::solid(base::EntityId pid, const Owner& owner)
{
    if (const auto known = m_solid.find(pid); known != m_solid.end())
        return known->second;
    SolidData data;
    const auto location = model().locate(base::IdSpace::Property, pid);
    if (!location)
        throw MassError(owner.text(model()) + " refers to property " + std::to_string(pid) + ", which does not exist");
    const std::uint16_t type = location->table;
    if (type == model::typeIndexOf<model::SolidPropTable>())
    {
        const model::SolidPropCard card = cardAt<model::SolidPropTable>(model(), *location);
        data.found = true;
        data.massless = card.elementFunction == "PressureFluid" || card.elementFunction == "FreeFluid";
        data.density = data.massless ? 0.0 : propertyDensity(card.matId, pid, owner);
        data.material = card.matId;
    }
    else if (type == model::typeIndexOf<model::SolidPropHyperelasticTable>())
    {
        const model::SolidPropHyperelasticCard card = cardAt<model::SolidPropHyperelasticTable>(model(), *location);
        data.found = true;
        data.density = propertyDensity(card.matId, pid, owner);
        data.material = card.matId;
    }
    else if (type == model::typeIndexOf<model::PlanePropNonlinearTable>())
    {
        const model::PlanePropNonlinearCard card = cardAt<model::PlanePropNonlinearTable>(model(), *location);
        data.found = true;
        data.density = propertyDensity(card.matId, pid, owner);
        data.material = card.matId;
    }
    return m_solid.emplace(pid, data).first->second;
}

const MassContext::LineData& MassContext::line(base::EntityId pid, const Owner& owner)
{
    if (const auto known = m_line.find(pid); known != m_line.end())
        return known->second;
    const auto location = model().locate(base::IdSpace::Property, pid);
    if (!location)
        throw MassError(owner.text(model()) + " refers to property " + std::to_string(pid) + ", which does not exist");
    std::optional<BeamSection> section = SectionFormulas::of(model(), *location, ShearFactorRule::None);
    if (!section)
    {
        throw MassError(owner.text(model()) + " refers to property " + std::to_string(pid) +
                        ", which is not a rod, tube, bar or beam property");
    }
    LineData data;
    data.density = propertyDensity(section->materialId, pid, owner);
    data.section = std::move(*section);
    return m_line.emplace(pid, std::move(data)).first->second;
}

double MassContext::lumpedMass(base::EntityId pid, const Owner& owner, bool required)
{
    if (const auto known = m_lumped.find(pid); known != m_lumped.end())
        return known->second;
    const auto location = model().locate(base::IdSpace::Property, pid);
    if (!location)
    {
        if (!required)
            return 0.0;
        throw MassError(owner.text(model()) + " refers to property " + std::to_string(pid) + ", which does not exist");
    }
    double mass = 0.0;
    const std::uint16_t type = location->table;
    if (type == model::typeIndexOf<model::MassPropTable>())
        mass = cardAt<model::MassPropTable>(model(), *location).mass;
    else if (type == model::typeIndexOf<model::BushPropTable>())
        mass = cardAt<model::BushPropTable>(model(), *location).mass;
    else if (type == model::typeIndexOf<model::FastenerPropTable>())
        mass = cardAt<model::FastenerPropTable>(model(), *location).mass;
    return m_lumped.emplace(pid, mass).first->second;
}

const MassRegistry& MassRegistry::instance()
{
    static const MassRegistry REGISTRY = []
    {
        MassRegistry registry;
        registerShellMass(registry);
        registerSolidMass(registry);
        registerLineMass(registry);
        registerPointMass(registry);
        return registry;
    }();
    return REGISTRY;
}

} // namespace oor::analysis::detail
