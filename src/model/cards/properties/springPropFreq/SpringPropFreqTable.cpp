#include "oor/model/cards/properties/springPropFreq/SpringPropFreqTable.hpp"

namespace oor::model
{

std::optional<SpringPropFreqCard> SpringPropFreqFormat::read(FieldReader& reader)
{
    SpringPropFreqCard card;
    card.id = reader.requiredInteger(1, "id");
    card.stiffnessTableId = reader.integer(2, "stiffnessTableId", 0);
    card.structuralDampingTableId = reader.integer(3, "structuralDampingTableId", 0);
    card.nonlinearForceTableId = reader.integer(4, "nonlinearForceTableId", 0);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has four fields",
                     "nonlinearForceTableId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SpringPropFreqFormat::check(const SpringPropFreqCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.stiffnessTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative",
                           "stiffnessTableId"};
    if (card.structuralDampingTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative",
                           "structuralDampingTableId"};
    if (card.nonlinearForceTableId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative",
                           "nonlinearForceTableId"};
    return std::nullopt;
}

void SpringPropFreqFormat::write(const SpringPropFreqCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.stiffnessTableId, 0);
    writer.integerUnlessDefault(3, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(4, card.nonlinearForceTableId, 0);
}

std::uint32_t SpringPropFreqTable::append(const SpringPropFreqCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffnessTableId.push(card.stiffnessTableId);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    m_nonlinearForceTableId.push(card.nonlinearForceTableId);
    return row;
}

SpringPropFreqCard SpringPropFreqTable::card(std::uint32_t row) const
{
    return SpringPropFreqCard{m_id.get(row), m_stiffnessTableId.get(row), m_structuralDampingTableId.get(row),
                              m_nonlinearForceTableId.get(row)};
}

void SpringPropFreqTable::store(std::uint32_t row, const SpringPropFreqCard& card)
{
    m_id.set(row, card.id);
    m_stiffnessTableId.set(row, card.stiffnessTableId);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
    m_nonlinearForceTableId.set(row, card.nonlinearForceTableId);
}

void SpringPropFreqTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffnessTableId.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
    m_nonlinearForceTableId.keepRows(rows);
}

void SpringPropFreqTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the PELAS that the card extends.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Property, m_id.get(row)));
    if (const base::EntityId stiffnessTableId = m_stiffnessTableId.get(row); stiffnessTableId > 0)
        m_stiffnessTableId.set(row, visitor.visit("stiffnessTableId", ReferenceTarget::DynamicTable, stiffnessTableId));
    if (const base::EntityId structuralDampingTableId = m_structuralDampingTableId.get(row);
        structuralDampingTableId > 0)
        m_structuralDampingTableId.set(
            row, visitor.visit("structuralDampingTableId", ReferenceTarget::DynamicTable, structuralDampingTableId));
    if (const base::EntityId nonlinearForceTableId = m_nonlinearForceTableId.get(row); nonlinearForceTableId > 0)
        m_nonlinearForceTableId.set(
            row, visitor.visit("nonlinearForceTableId", ReferenceTarget::DynamicTable, nonlinearForceTableId));
}

} // namespace oor::model
