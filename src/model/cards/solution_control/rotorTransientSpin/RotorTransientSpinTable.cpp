#include "oor/model/cards/solution_control/rotorTransientSpin/RotorTransientSpinTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"

namespace oor::model
{

std::optional<RotorTransientSpinCard> RotorTransientSpinFormat::read(FieldReader& reader)
{
    RotorTransientSpinCard card;
    card.rotorId = reader.requiredInteger(1, "rotorId");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.speedUnit = readRequiredKeyword<RotorSpeedUnit>(reader, 4, "speedUnit", ROTOR_SPEED_UNIT_WORDS);
    card.spinTableId = reader.requiredInteger(5, "spinTableId");
    card.speedOutput = reader.integer(6, "speedOutput", 0);
    card.rotorSuperelementId = reader.integer(7, "rotorSuperelementId", 0);
    card.node = reader.real(9, "node", 0.0);
    card.massDampingFactor = reader.real(10, "massDampingFactor", 0.0);
    card.stiffnessDampingFactor = reader.real(11, "stiffnessDampingFactor", 0.0);
    card.viscousDampingFrequency = reader.real(12, "viscousDampingFrequency", 0.0);
    card.structuralDampingFrequency = reader.real(13, "structuralDampingFrequency", 0.0);
    card.hybridDampingFrequency = reader.real(14, "hybridDampingFrequency", 0.0);
    requireOnlyFields(reader, {1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12, 13, 14});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorTransientSpinFormat::check(const RotorTransientSpinCard& card)
{
    if (const auto problem = checkRequiredId(card.rotorId, "rotorId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeB, "nodeB"))
        return problem;
    if (const auto problem = checkRequiredId(card.spinTableId, "spinTableId"))
        return problem;
    if (const auto problem = checkOptionalId(card.speedOutput, "speedOutput"))
        return problem;
    if (card.rotorSuperelementId < 0)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must not be negative (0 is the residual structure)",
                           "rotorSuperelementId"};
    if (const auto problem = checkReal(card.node, "node", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.massDampingFactor, "massDampingFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.stiffnessDampingFactor, "stiffnessDampingFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.viscousDampingFrequency, "viscousDampingFrequency", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.structuralDampingFrequency, "structuralDampingFrequency", RealRule::Finite))
        return problem;
    return checkReal(card.hybridDampingFrequency, "hybridDampingFrequency", RealRule::Finite);
}

void RotorTransientSpinFormat::write(const RotorTransientSpinCard& card, FieldWriter& writer)
{
    writer.integer(1, card.rotorId);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writeKeyword(writer, 4, ROTOR_SPEED_UNIT_WORDS, card.speedUnit);
    writer.integer(5, card.spinTableId);
    writer.integerUnlessDefault(6, card.speedOutput, 0);
    writer.integerUnlessDefault(7, card.rotorSuperelementId, 0);
    writer.realUnlessDefault(9, card.node, 0.0);
    writer.realUnlessDefault(10, card.massDampingFactor, 0.0);
    writer.realUnlessDefault(11, card.stiffnessDampingFactor, 0.0);
    writer.realUnlessDefault(12, card.viscousDampingFrequency, 0.0);
    writer.realUnlessDefault(13, card.structuralDampingFrequency, 0.0);
    writer.realUnlessDefault(14, card.hybridDampingFrequency, 0.0);
}

std::uint32_t RotorTransientSpinTable::append(const RotorTransientSpinCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_rotorId.push(card.rotorId);
    m_rotorSuperelementId.push(card.rotorSuperelementId);
    m_cards.push(card);
    return row;
}

void RotorTransientSpinTable::store(std::uint32_t row, const RotorTransientSpinCard& card)
{
    m_rotorId.set(row, card.rotorId);
    m_rotorSuperelementId.set(row, card.rotorSuperelementId);
    m_cards.set(row, card);
}

void RotorTransientSpinTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_rotorId.keepRows(rows);
    m_rotorSuperelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorTransientSpinTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RotorTransientSpinCard& card = m_cards.edit(row);
    // The rotor and its grid points are in the residual structure only when ROTRSEID is 0.
    visitRotor(visitor, "rotorId", card.rotorId, card.rotorSuperelementId);
    m_rotorId.set(row, card.rotorId);
    if (card.rotorSuperelementId == 0)
    {
        visitId(visitor, "nodeA", ReferenceTarget::Node, card.nodeA);
        visitId(visitor, "nodeB", ReferenceTarget::Node, card.nodeB);
    }
    visitId(visitor, "spinTableId", ReferenceTarget::DynamicTable, card.spinTableId);
    visitId(visitor, "speedOutput", ReferenceTarget::Node, card.speedOutput);
}

} // namespace oor::model
