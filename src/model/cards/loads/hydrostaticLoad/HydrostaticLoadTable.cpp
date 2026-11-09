#include "oor/model/cards/loads/hydrostaticLoad/HydrostaticLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/loads/HydrostaticSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 12;    // CNTL, the fourth field of the second line
constexpr int HIGHEST_SWITCH = 1; // PLD4 and OVRD are off or on

} // namespace

std::optional<HydrostaticLoadCard> HydrostaticLoadFormat::read(FieldReader& reader)
{
    HydrostaticLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nominalPressure = reader.real(2, "nominalPressure", 0.0);
    card.density = reader.requiredReal(3, "density");
    card.gravity = reader.real(4, "gravity", 1.0);
    card.elementSetId = reader.integer(5, "elementSetId", 0);
    card.nodeSetId = reader.integer(6, "nodeSetId", 0);
    card.punchPressureLoads = reader.integer(7, "punchPressureLoads", 0);
    card.overrideOption = reader.integer(8, "overrideOption", 0);
    card.frameOrNode = reader.integer(9, "frameId/nodeAlt", 0);
    card.baseHeight = reader.real(10, "baseHeight", 0.0);
    card.heightAbove = reader.real(11, "heightAbove", 0.0);
    card.controlNodeSetId = reader.integer(12, "controlNodeSetId", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("controlNodeSetId"),
                     "controlNodeSetId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HydrostaticLoadFormat::check(const HydrostaticLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.nominalPressure, "nominalPressure", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.density, "density", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.gravity, "gravity", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.elementSetId, "elementSetId"))
        return problem;
    if (const auto problem = checkOptionalId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkHydrostaticOption(card.punchPressureLoads, "punchPressureLoads", HIGHEST_SWITCH))
        return problem;
    if (const auto problem = checkHydrostaticOption(card.overrideOption, "overrideOption", HIGHEST_SWITCH))
        return problem;
    if (const auto problem = checkHydrostaticReference(card.frameOrNode))
        return problem;
    if (const auto problem = checkReal(card.baseHeight, "baseHeight", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.heightAbove, "heightAbove", RealRule::Finite))
        return problem;
    return checkOptionalId(card.controlNodeSetId, "controlNodeSetId");
}

void HydrostaticLoadFormat::write(const HydrostaticLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.nominalPressure, 0.0);
    writer.real(3, card.density);
    writer.realUnlessDefault(4, card.gravity, 1.0);
    writer.integerUnlessDefault(5, card.elementSetId, 0);
    writer.integerUnlessDefault(6, card.nodeSetId, 0);
    writer.integerUnlessDefault(7, card.punchPressureLoads, 0);
    writer.integerUnlessDefault(8, card.overrideOption, 0);
    writer.integerUnlessDefault(9, card.frameOrNode, 0);
    writer.realUnlessDefault(10, card.baseHeight, 0.0);
    writer.realUnlessDefault(11, card.heightAbove, 0.0);
    writer.integerUnlessDefault(12, card.controlNodeSetId, 0);
}

std::uint32_t HydrostaticLoadTable::append(const HydrostaticLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nominalPressure.push(card.nominalPressure);
    m_density.push(card.density);
    m_gravity.push(card.gravity);
    m_elementSetId.push(card.elementSetId);
    m_nodeSetId.push(card.nodeSetId);
    m_punchPressureLoads.push(card.punchPressureLoads);
    m_overrideOption.push(card.overrideOption);
    m_frameOrNode.push(card.frameOrNode);
    m_baseHeight.push(card.baseHeight);
    m_heightAbove.push(card.heightAbove);
    m_controlNodeSetId.push(card.controlNodeSetId);
    return row;
}

HydrostaticLoadCard HydrostaticLoadTable::card(std::uint32_t row) const
{
    HydrostaticLoadCard card;
    card.id = m_id.get(row);
    card.nominalPressure = m_nominalPressure.get(row);
    card.density = m_density.get(row);
    card.gravity = m_gravity.get(row);
    card.elementSetId = m_elementSetId.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.punchPressureLoads = m_punchPressureLoads.get(row);
    card.overrideOption = m_overrideOption.get(row);
    card.frameOrNode = m_frameOrNode.get(row);
    card.baseHeight = m_baseHeight.get(row);
    card.heightAbove = m_heightAbove.get(row);
    card.controlNodeSetId = m_controlNodeSetId.get(row);
    return card;
}

void HydrostaticLoadTable::store(std::uint32_t row, const HydrostaticLoadCard& card)
{
    m_id.set(row, card.id);
    m_nominalPressure.set(row, card.nominalPressure);
    m_density.set(row, card.density);
    m_gravity.set(row, card.gravity);
    m_elementSetId.set(row, card.elementSetId);
    m_nodeSetId.set(row, card.nodeSetId);
    m_punchPressureLoads.set(row, card.punchPressureLoads);
    m_overrideOption.set(row, card.overrideOption);
    m_frameOrNode.set(row, card.frameOrNode);
    m_baseHeight.set(row, card.baseHeight);
    m_heightAbove.set(row, card.heightAbove);
    m_controlNodeSetId.set(row, card.controlNodeSetId);
}

void HydrostaticLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nominalPressure.keepRows(rows);
    m_density.keepRows(rows);
    m_gravity.keepRows(rows);
    m_elementSetId.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_punchPressureLoads.keepRows(rows);
    m_overrideOption.keepRows(rows);
    m_frameOrNode.keepRows(rows);
    m_baseHeight.keepRows(rows);
    m_heightAbove.keepRows(rows);
    m_controlNodeSetId.keepRows(rows);
}

void HydrostaticLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The three sets of the card, in the order of its fields; a blank one names nothing.
    if (const base::EntityId elementSetId = m_elementSetId.get(row); elementSetId > 0)
        m_elementSetId.set(row, visitor.visit("elementSetId", ReferenceTarget::Set, elementSetId));
    if (const base::EntityId nodeSetId = m_nodeSetId.get(row); nodeSetId > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, nodeSetId));
    // A coordinate system above zero, a grid point below zero (the sign stays), nothing for the basic system.
    base::EntityId frameOrNode = m_frameOrNode.get(row);
    visitHydrostaticReference(visitor, "frameId/nodeAlt", frameOrNode);
    m_frameOrNode.set(row, frameOrNode);
    if (const base::EntityId controlNodeSetId = m_controlNodeSetId.get(row); controlNodeSetId > 0)
        m_controlNodeSetId.set(row, visitor.visit("controlNodeSetId", ReferenceTarget::Set, controlNodeSetId));
}

} // namespace oor::model
