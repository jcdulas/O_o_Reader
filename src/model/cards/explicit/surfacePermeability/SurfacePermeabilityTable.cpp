#include "oor/model/cards/explicit/surfacePermeability/SurfacePermeabilityTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<SurfacePermeabilityCard> SurfacePermeabilityFormat::read(FieldReader& reader)
{
    SurfacePermeabilityCard card;
    card.id = reader.requiredInteger(1, "id");
    card.permeability = reader.optionalReal(2, "permeability");
    card.permeabilityTableId = reader.integer(3, "permeabilityTableId", 0);
    card.flow = readKeyword(reader, 4, "flowDirection", FLOW_DIRECTION_WORDS, FlowDirection::Both);
    card.environmentPressure = reader.requiredReal(5, "environmentPressure");
    card.environmentDensity = reader.requiredReal(6, "environmentDensity");
    card.environmentEnergy = reader.requiredReal(7, "environmentEnergy");
    card.environmentSpecificHeat = reader.optionalReal(8, "environmentSpecificHeat");
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 8 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SurfacePermeabilityFormat::check(const SurfacePermeabilityCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.permeability, "permeability", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.permeabilityTableId, "permeabilityTableId"))
        return problem;
    // The permeability is a linear law or a table, so one of the two must be there.
    if (!card.permeability && card.permeabilityTableId == 0)
        return CardProblem{
            "card.missing-field",
            std::string(CardProblem::FIELD) + " or " + CardProblem::fieldNamed("permeabilityTableId") + " is required",
            "permeability"};
    if (const auto problem = checkReal(card.environmentPressure, "environmentPressure", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.environmentDensity, "environmentDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.environmentEnergy, "environmentEnergy", RealRule::Positive))
        return problem;
    return checkOptionalReal(card.environmentSpecificHeat, "environmentSpecificHeat", RealRule::Positive);
}

void SurfacePermeabilityFormat::write(const SurfacePermeabilityCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.permeability)
        writer.real(2, *card.permeability);
    writer.integerUnlessDefault(3, card.permeabilityTableId, 0);
    writeKeywordUnlessDefault(writer, 4, FLOW_DIRECTION_WORDS, card.flow, FlowDirection::Both);
    writer.real(5, card.environmentPressure);
    writer.real(6, card.environmentDensity);
    writer.real(7, card.environmentEnergy);
    if (card.environmentSpecificHeat)
        writer.real(8, *card.environmentSpecificHeat);
}

std::uint32_t SurfacePermeabilityTable::append(const SurfacePermeabilityCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SurfacePermeabilityTable::store(std::uint32_t row, const SurfacePermeabilityCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SurfacePermeabilityTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SurfacePermeabilityTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SurfacePermeabilityTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "permeabilityTableId", ReferenceTarget::DynamicTable, m_cards.edit(row).permeabilityTableId);
}

} // namespace oor::model
