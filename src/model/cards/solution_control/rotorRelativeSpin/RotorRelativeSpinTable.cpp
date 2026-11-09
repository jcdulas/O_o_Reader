#include "oor/model/cards/solution_control/rotorRelativeSpin/RotorRelativeSpinTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/RotorSupport.hpp"

#include <variant>

namespace oor::model
{

namespace
{

constexpr int ROTOR_SUPERELEMENT_FIELD = 6;

} // namespace

std::optional<RotorRelativeSpinCard> RotorRelativeSpinFormat::read(FieldReader& reader)
{
    RotorRelativeSpinCard card;
    card.rotorId = reader.requiredInteger(1, "rotorId");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.speedUnit = readRequiredKeyword<RotorSpeedUnit>(reader, 4, "speedUnit", ROTOR_SPEED_UNIT_WORDS);
    // SPTID is required: a real is a constant rate, an integer the number of a DDVAL.
    if (const auto rate = reader.integerOrReal(5, "spinTableId"))
    {
        if (std::holds_alternative<std::int32_t>(*rate))
        {
            card.spinTableId.table = std::get<std::int32_t>(*rate);
            if (card.spinTableId.table <= 0)
                reader.error("card.bad-id", std::string("spinTableId") + ", when it is an integer, must be positive",
                             "spinTableId");
        }
        else
        {
            card.spinTableId.constant = std::get<double>(*rate);
        }
    }
    else if (reader.isBlank(5))
    {
        reader.error("card.missing-field",
                     std::string("spinTableId") + " is required: a real (a constant rate) or the number of a " +
                         std::string("DesignDiscreteValues"),
                     "spinTableId");
    }
    card.rotorSuperelementId = reader.integer(ROTOR_SUPERELEMENT_FIELD, "rotorSuperelementId", 0);
    card.node = reader.real(9, "node", 0.0);
    card.massDampingFactor = reader.real(10, "massDampingFactor", 0.0);
    card.stiffnessDampingFactor = reader.real(11, "stiffnessDampingFactor", 0.0);
    card.viscousDampingFrequency = reader.real(12, "viscousDampingFrequency", 0.0);
    card.structuralDampingFrequency = reader.real(13, "structuralDampingFrequency", 0.0);
    card.hybridDampingFrequency = reader.real(14, "hybridDampingFrequency", 0.0);
    requireOnlyFields(reader, {1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14});
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorRelativeSpinFormat::check(const RotorRelativeSpinCard& card)
{
    if (const auto problem = checkRequiredId(card.rotorId, "rotorId"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeA, "nodeA"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeB, "nodeB"))
        return problem;
    if (card.spinTableId.table < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + ", when it is an integer, must be positive",
                           "spinTableId"};
    if (const auto problem = checkReal(card.spinTableId.constant, "spinTableId", RealRule::Finite))
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

void RotorRelativeSpinFormat::write(const RotorRelativeSpinCard& card, FieldWriter& writer)
{
    writer.integer(1, card.rotorId);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writeKeyword(writer, 4, ROTOR_SPEED_UNIT_WORDS, card.speedUnit);
    if (card.spinTableId.isTable())
        writer.integer(5, card.spinTableId.table);
    else
        writer.real(5, card.spinTableId.constant);
    writer.integerUnlessDefault(ROTOR_SUPERELEMENT_FIELD, card.rotorSuperelementId, 0);
    writer.realUnlessDefault(9, card.node, 0.0);
    writer.realUnlessDefault(10, card.massDampingFactor, 0.0);
    writer.realUnlessDefault(11, card.stiffnessDampingFactor, 0.0);
    writer.realUnlessDefault(12, card.viscousDampingFrequency, 0.0);
    writer.realUnlessDefault(13, card.structuralDampingFrequency, 0.0);
    writer.realUnlessDefault(14, card.hybridDampingFrequency, 0.0);
}

std::uint32_t RotorRelativeSpinTable::append(const RotorRelativeSpinCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_rotorId.push(card.rotorId);
    m_rotorSuperelementId.push(card.rotorSuperelementId);
    m_cards.push(card);
    return row;
}

void RotorRelativeSpinTable::store(std::uint32_t row, const RotorRelativeSpinCard& card)
{
    m_rotorId.set(row, card.rotorId);
    m_rotorSuperelementId.set(row, card.rotorSuperelementId);
    m_cards.set(row, card);
}

void RotorRelativeSpinTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_rotorId.keepRows(rows);
    m_rotorSuperelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorRelativeSpinTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RotorRelativeSpinCard& card = m_cards.edit(row);
    // The rotor and its grid points are in the residual structure only when ROTRSEID is 0.
    visitRotor(visitor, "rotorId", card.rotorId, card.rotorSuperelementId);
    m_rotorId.set(row, card.rotorId);
    if (card.rotorSuperelementId == 0)
    {
        visitId(visitor, "nodeA", ReferenceTarget::Node, card.nodeA);
        visitId(visitor, "nodeB", ReferenceTarget::Node, card.nodeB);
    }
    if (card.spinTableId.isTable())
        visitId(visitor, "spinTableId", ReferenceTarget::DiscreteValueSet, card.spinTableId.table);
}

} // namespace oor::model
