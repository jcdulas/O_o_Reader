#include "oor/analysis/components/EntityDofs.hpp"

#include "oor/analysis/connections/RigidElementTerms.hpp"
#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/damperDofProperty/DamperDofPropertyTable.hpp"
#include "oor/model/cards/elements/damperDofInline/DamperDofInlineTable.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/properties/bushProp/BushPropTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"

#include <algorithm>
#include <initializer_list>
#include <string_view>

namespace oor::analysis
{

namespace
{

constexpr ComponentSet TRANSLATIONS = 0b0000'1110;
constexpr ComponentSet ALL_SIX = 0b0111'1110;
constexpr ComponentSet SCALAR = 0b0000'0001;

bool startsWithAny(std::string_view name, std::initializer_list<std::string_view> prefixes) noexcept
{
    return std::any_of(prefixes.begin(), prefixes.end(),
                       [&](std::string_view prefix) { return name.starts_with(prefix); });
}

// The components of a two-point scalar element at a node: C1 at G1, C2 at G2.
template <typename TableT>
ComponentSet scalarComponents(const model::Model& model, model::CardLocation card, base::EntityId node)
{
    const auto value = model.findTable<TableT>()->card(card.row);
    ComponentSet components = 0;
    if (value.node1 == node)
        components |= EntityDofs::fromDigits(value.components1);
    if (value.node2 == node)
        components |= EntityDofs::fromDigits(value.components2);
    return components;
}

// The components of a rod: a rod carries an axial force and, with a torsion constant J, a torque; without one (J blank
// or 0, format reference PROD and CONROD) it acts on the translations only. A CTUBE always has a torsion constant (from
// its section).
ComponentSet rodComponents(const model::Model& model, model::CardLocation card, std::string_view name)
{
    double torsion = 0.0;
    if (name == "Rod2InlineSection")
        torsion = model.findTable<model::Rod2InlineSectionTable>()->card(card.row).torsionConstant;
    else
    {
        const model::Rod2Card rod = model.findTable<model::Rod2Table>()->card(card.row);
        const auto property = model.locate(base::IdSpace::Property, rod.propId);
        if (!property || property->table != model::typeIndexOf<model::RodPropTable>())
            return ALL_SIX; // a property the model does not have: nothing tells what the rod carries
        torsion = model.findTable<model::RodPropTable>()->card(property->row).torsion;
    }
    return torsion != 0.0 ? ALL_SIX : TRANSLATIONS;
}

ComponentSet bushComponents(const model::Model& model, model::CardLocation card)
{
    const model::BushingCard bush = model.findTable<model::BushingTable>()->card(card.row);
    const auto property = model.locate(base::IdSpace::Property, bush.propId);
    if (!property || property->table != model::typeIndexOf<model::BushPropTable>())
        return ALL_SIX;
    const model::BushPropCard values = model.findTable<model::BushPropTable>()->card(property->row);
    ComponentSet components = 0;
    for (std::size_t index = 0; index < 6; ++index)
    {
        if (values.stiffness[index] != 0.0)
            components |= static_cast<ComponentSet>(1U << (index + 1));
    }
    return components;
}

} // namespace

ComponentSet EntityDofs::fromDigits(std::int32_t digits) noexcept
{
    if (digits <= 0)
        return SCALAR;
    ComponentSet components = 0;
    while (digits > 0)
    {
        const std::int32_t digit = digits % 10;
        if (digit >= 1 && digit <= 6)
            components |= static_cast<ComponentSet>(1U << digit);
        else if (digit == 0)
            components |= SCALAR;
        digits /= 10;
    }
    return components;
}

std::string EntityDofs::text(ComponentSet components)
{
    std::string digits;
    for (int component = 0; component <= 6; ++component)
    {
        if ((components & (1U << component)) != 0)
            digits.push_back(static_cast<char>('0' + component));
    }
    return digits;
}

ComponentSet EntityDofs::at(const model::Model& model, const ModelEntity& entity, base::EntityId node,
                            const FastenerPiercings* piercings)
{
    if (entity.isEquation)
    {
        ComponentSet components = 0;
        for (const model::MultiPointConstrTerm& term :
             model.findTable<model::MultiPointConstrTable>()->card(entity.card.row).terms)
        {
            if (term.node == node)
                components |= fromDigits(term.component);
        }
        return components;
    }
    const std::string_view name = model.findTable(entity.card.table)->info().name;
    switch (entity.role)
    {
    case ElementRole::Solid:
        return TRANSLATIONS;
    case ElementRole::Shell:
        return startsWithAny(name, {"AxisymQuad", "AxisymTria", "PlaneStrain", "PlaneStress"})
                   ? TRANSLATIONS
                   : ALL_SIX;
    case ElementRole::Line:
        return name == "Rod2" || name == "Rod2InlineSection" ? rodComponents(model, entity.card, name) : ALL_SIX;
    case ElementRole::Mass:
        if (name == "MassDofProperty")
            return scalarComponents<model::MassDofPropertyTable>(model, entity.card, node);
        if (name == "MassDofInline")
            return scalarComponents<model::MassDofInlineTable>(model, entity.card, node);
        if (name == "MassScalarProperty" || name == "MassScalarInline")
            return SCALAR;
        return ALL_SIX;
    case ElementRole::Rigid:
    {
        ComponentSet components = 0;
        for (const RigidTerm& term : RigidElementTerms::of(model, entity.card))
        {
            if (term.node == node)
                components |= term.components == 0 ? TRANSLATIONS : fromDigits(term.components);
        }
        return components;
    }
    case ElementRole::Connector:
    {
        if (name == "Fastener" || name == "Weld")
        {
            const PiercedFastener* fastener = piercings != nullptr ? piercings->find(entity.card) : nullptr;
            if (fastener != nullptr && !std::binary_search(fastener->ownNodes.begin(), fastener->ownNodes.end(), node))
                return TRANSLATIONS;
            return ALL_SIX;
        }
        if (name == "SpringDofProperty")
            return scalarComponents<model::SpringDofPropertyTable>(model, entity.card, node);
        if (name == "SpringDofInline")
            return scalarComponents<model::SpringDofInlineTable>(model, entity.card, node);
        if (name == "DamperDofProperty")
            return scalarComponents<model::DamperDofPropertyTable>(model, entity.card, node);
        if (name == "DamperDofInline")
            return scalarComponents<model::DamperDofInlineTable>(model, entity.card, node);
        if (name == "SpringScalarProperty" || name == "SpringScalarInline" || name == "DamperScalarProperty" || name == "DamperScalarInline" ||
            name == "HeatCapacity2")
            return SCALAR;
        if (name == "Bushing")
            return bushComponents(model, entity.card);
        if (name == "Gap2" || name == "Bushing1D")
            return TRANSLATIONS;
        return ALL_SIX;
    }
    case ElementRole::None:
        break;
    }
    return ALL_SIX;
}

} // namespace oor::analysis
