#include "oor/model/cards/loads/hydrostaticCavityLoad/HydrostaticCavityLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/HydrostaticSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int UNUSED_FIELD = 8;         // the last cell of the first line
constexpr int LAST_FIELD = 11;          // HABOV, the third field of the second line
constexpr int HIGHEST_PUNCH_OPTION = 3; // what is punched: nothing, the grids, the elements, both
constexpr int HIGHEST_SWITCH = 1;       // PLD4 and OVRD are off or on

} // namespace

std::optional<HydrostaticCavityLoadCard> HydrostaticCavityLoadFormat::read(FieldReader& reader)
{
    HydrostaticCavityLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.nominalPressure = reader.real(3, "nominalPressure", 0.0);
    card.gravity = reader.real(4, "gravity", 1.0);
    card.punchOption = reader.integer(5, "punchOption", 0);
    card.punchPressureLoads = reader.integer(6, "punchPressureLoads", 0);
    card.overrideOption = reader.integer(7, "overrideOption", 0);
    requireBlankFields(reader, {UNUSED_FIELD}, LAST_FIELD);
    card.frameOrNode = reader.integer(9, "frameId/nodeAlt", 0);
    card.baseHeight = reader.real(10, "baseHeight", 0.0);
    card.heightAbove = reader.real(11, "heightAbove", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HydrostaticCavityLoadFormat::check(const HydrostaticCavityLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkReal(card.nominalPressure, "nominalPressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.gravity, "gravity", RealRule::Finite))
        return problem;
    if (const auto problem = checkHydrostaticOption(card.punchOption, "punchOption", HIGHEST_PUNCH_OPTION))
        return problem;
    if (const auto problem = checkHydrostaticOption(card.punchPressureLoads, "punchPressureLoads", HIGHEST_SWITCH))
        return problem;
    if (const auto problem = checkHydrostaticOption(card.overrideOption, "overrideOption", HIGHEST_SWITCH))
        return problem;
    if (const auto problem = checkHydrostaticReference(card.frameOrNode))
        return problem;
    if (const auto problem = checkReal(card.baseHeight, "baseHeight", RealRule::Finite))
        return problem;
    return checkReal(card.heightAbove, "heightAbove", RealRule::Finite);
}

void HydrostaticCavityLoadFormat::write(const HydrostaticCavityLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.realUnlessDefault(3, card.nominalPressure, 0.0);
    writer.realUnlessDefault(4, card.gravity, 1.0);
    writer.integerUnlessDefault(5, card.punchOption, 0);
    writer.integerUnlessDefault(6, card.punchPressureLoads, 0);
    writer.integerUnlessDefault(7, card.overrideOption, 0);
    writer.integerUnlessDefault(9, card.frameOrNode, 0);
    writer.realUnlessDefault(10, card.baseHeight, 0.0);
    writer.realUnlessDefault(11, card.heightAbove, 0.0);
}

std::uint32_t HydrostaticCavityLoadTable::append(const HydrostaticCavityLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nominalPressure.push(card.nominalPressure);
    m_gravity.push(card.gravity);
    m_punchOption.push(card.punchOption);
    m_punchPressureLoads.push(card.punchPressureLoads);
    m_overrideOption.push(card.overrideOption);
    m_frameOrNode.push(card.frameOrNode);
    m_baseHeight.push(card.baseHeight);
    m_heightAbove.push(card.heightAbove);
    return row;
}

HydrostaticCavityLoadCard HydrostaticCavityLoadTable::card(std::uint32_t row) const
{
    HydrostaticCavityLoadCard card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.nominalPressure = m_nominalPressure.get(row);
    card.gravity = m_gravity.get(row);
    card.punchOption = m_punchOption.get(row);
    card.punchPressureLoads = m_punchPressureLoads.get(row);
    card.overrideOption = m_overrideOption.get(row);
    card.frameOrNode = m_frameOrNode.get(row);
    card.baseHeight = m_baseHeight.get(row);
    card.heightAbove = m_heightAbove.get(row);
    return card;
}

void HydrostaticCavityLoadTable::store(std::uint32_t row, const HydrostaticCavityLoadCard& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nominalPressure.set(row, card.nominalPressure);
    m_gravity.set(row, card.gravity);
    m_punchOption.set(row, card.punchOption);
    m_punchPressureLoads.set(row, card.punchPressureLoads);
    m_overrideOption.set(row, card.overrideOption);
    m_frameOrNode.set(row, card.frameOrNode);
    m_baseHeight.set(row, card.baseHeight);
    m_heightAbove.set(row, card.heightAbove);
}

void HydrostaticCavityLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nominalPressure.keepRows(rows);
    m_gravity.keepRows(rows);
    m_punchOption.keepRows(rows);
    m_punchPressureLoads.keepRows(rows);
    m_overrideOption.keepRows(rows);
    m_frameOrNode.keepRows(rows);
    m_baseHeight.keepRows(rows);
    m_heightAbove.keepRows(rows);
}

void HydrostaticCavityLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, propId));
    // A coordinate system above zero, a grid point below zero (the sign stays), nothing for the basic system.
    base::EntityId frameOrNode = m_frameOrNode.get(row);
    visitHydrostaticReference(visitor, "frameId/nodeAlt", frameOrNode);
    m_frameOrNode.set(row, frameOrNode);
}

} // namespace oor::model
