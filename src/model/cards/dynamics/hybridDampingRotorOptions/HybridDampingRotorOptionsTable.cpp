#include "oor/model/cards/dynamics/hybridDampingRotorOptions/HybridDampingRotorOptionsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<HybridDampingRotorOptionsCard> HybridDampingRotorOptionsFormat::read(FieldReader& reader)
{
    HybridDampingRotorOptionsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.structuralDamping = reader.real(2, "structuralDamping", 0.0);
    card.alphaCoefficient1 = reader.real(3, "alphaCoefficient1", 0.0);
    card.alphaCoefficient2 = reader.real(4, "alphaCoefficient2", 0.0);
    card.hybridDampingId = reader.integer(5, "hybridDampingId", 0);
    card.structuralDampingFactor = reader.real(6, "structuralDampingFactor", 1.0);
    card.rotorSeparation = readKeyword(reader, 7, "rotorSeparation", ROTOR_SEPARATION_WORDS, RotorSeparation::Yes);
    card.structuralDampingFrequency = reader.real(9, "structuralDampingFrequency", 0.0);
    card.materialDampingFrequency = reader.real(10, "materialDampingFrequency", 0.0);
    card.hybridDampingFrequency = reader.real(11, "hybridDampingFrequency", 0.0);
    requireUnusedBlank(reader, {8}, 11); // the cell after ROTSEP, and the end of the card
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> HybridDampingRotorOptionsFormat::check(const HybridDampingRotorOptionsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.structuralDamping, "structuralDamping", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.alphaCoefficient1, "alphaCoefficient1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.alphaCoefficient2, "alphaCoefficient2", RealRule::Finite))
        return problem;
    if (const auto problem = checkOptionalId(card.hybridDampingId, "hybridDampingId"))
        return problem;
    if (const auto problem = checkReal(card.structuralDampingFactor, "structuralDampingFactor", RealRule::Finite))
        return problem;
    if (const auto problem =
            checkReal(card.structuralDampingFrequency, "structuralDampingFrequency", RealRule::NonNegative))
        return problem;
    if (const auto problem =
            checkReal(card.materialDampingFrequency, "materialDampingFrequency", RealRule::NonNegative))
        return problem;
    return checkReal(card.hybridDampingFrequency, "hybridDampingFrequency", RealRule::NonNegative);
}

void HybridDampingRotorOptionsFormat::write(const HybridDampingRotorOptionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.structuralDamping, 0.0);
    writer.realUnlessDefault(3, card.alphaCoefficient1, 0.0);
    writer.realUnlessDefault(4, card.alphaCoefficient2, 0.0);
    writer.integerUnlessDefault(5, card.hybridDampingId, 0);
    writer.realUnlessDefault(6, card.structuralDampingFactor, 1.0);
    writeKeywordUnlessDefault(writer, 7, ROTOR_SEPARATION_WORDS, card.rotorSeparation, RotorSeparation::Yes);
    writer.realUnlessDefault(9, card.structuralDampingFrequency, 0.0);
    writer.realUnlessDefault(10, card.materialDampingFrequency, 0.0);
    writer.realUnlessDefault(11, card.hybridDampingFrequency, 0.0);
}

std::uint32_t HybridDampingRotorOptionsTable::append(const HybridDampingRotorOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void HybridDampingRotorOptionsTable::store(std::uint32_t row, const HybridDampingRotorOptionsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void HybridDampingRotorOptionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void HybridDampingRotorOptionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void HybridDampingRotorOptionsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "hybridDampingId", ReferenceTarget::HybridDamping, m_cards.edit(row).hybridDampingId);
}

} // namespace oor::model
