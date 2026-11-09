#include "oor/analysis/components/ElementRoles.hpp"

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/model/core/CardRegistry.hpp"

#include <algorithm>
#include <initializer_list>
#include <vector>

namespace oor::analysis
{

namespace
{

bool isAny(std::string_view name, std::initializer_list<std::string_view> names) noexcept
{
    return std::find(names.begin(), names.end(), name) != names.end();
}

} // namespace

ElementRole roleOfCard(std::string_view name) noexcept
{
    if (isAny(name, {"RigidBar", "RigidBar1", "RigidAxisymConnection", "RigidBodyMulti", "RigidBodyStar",
                     "RigidInterpolation", "RigidJoint", "RigidRod", "RigidSpline", "RigidShellSolidConnection",
                     "RigidTriaPlate", "RigidTriaPlate1"}))
        return ElementRole::Rigid;
    if (isAny(name, {"PointMassMatrix", "PointMassInertia", "MassDofProperty", "MassDofInline", "MassScalarProperty", "MassScalarInline"}))
        return ElementRole::Mass;
    if (isAny(name, {"Rod2", "Rod2InlineSection", "Tube2", "Bar2", "Beam2", "Beam3", "CurvedBeam2"}))
        return ElementRole::Line;
    if (isAny(name, {"Bushing",
                     "Bushing1D",
                     "Fastener",
                     "Weld",
                     "SpringDofProperty",
                     "SpringDofInline",
                     "SpringScalarProperty",
                     "SpringScalarInline",
                     "DamperDofProperty",
                     "DamperDofInline",
                     "DamperScalarProperty",
                     "DamperScalarInline",
                     "HeatCapacity2",
                     "Gap2",
                     "ViscousDamper2",
                     "GeneralMatrix",
                     "Bushing2D",
                     "SpringDofPropertyExplicit",
                     "SpringDofInlineExplicit",
                     "DamperDofPropertyExplicit",
                     "DamperDofInlineExplicit",
                     "SpringOffsetExplicit",
                     "Seam"}))
        return ElementRole::Connector;
    return ElementRole::None;
}

ElementRole roleOfType(std::uint16_t typeIndex) noexcept
{
    static const std::vector<ElementRole> ROLES = []
    {
        const model::CardRegistry& registry = model::CardRegistry::instance();
        std::vector<ElementRole> roles(registry.size(), ElementRole::None);
        for (std::size_t type = 1; type < registry.size(); ++type)
        {
            const model::CardTypeInfo& info = registry.entry(static_cast<std::uint16_t>(type)).info;
            if (info.category != model::CardCategory::Element)
                continue;
            const ElementRole byName = roleOfCard(info.name);
            if (byName != ElementRole::None)
            {
                roles[type] = byName;
                continue;
            }
            // Anything else with a shape is a shell or a solid by its dimension.
            const int dimension = dimensionOf(ElementTopology::shapeOf(static_cast<std::uint16_t>(type)));
            if (dimension == 2)
                roles[type] = ElementRole::Shell;
            else if (dimension == 3)
                roles[type] = ElementRole::Solid;
        }
        return roles;
    }();
    return typeIndex < ROLES.size() ? ROLES[typeIndex] : ElementRole::None;
}

} // namespace oor::analysis
