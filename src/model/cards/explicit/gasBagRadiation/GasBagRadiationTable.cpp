#include "oor/model/cards/explicit/gasBagRadiation/GasBagRadiationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<GasBagRadiationCard> GasBagRadiationFormat::read(FieldReader& reader)
{
    GasBagRadiationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.emissivity = reader.optionalReal(2, "emissivity");
    card.emissivityTableId = reader.integer(3, "emissivityTableId", 0);
    card.environmentTemperature = reader.requiredReal(4, "environmentTemperature");
    card.stefanBoltzmann = reader.requiredReal(5, "stefanBoltzmann");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagRadiationFormat::check(const GasBagRadiationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.emissivity, "emissivity", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.emissivityTableId, "emissivityTableId"))
        return problem;
    // The emissivity is the constant or the table, one of the two.
    if (!card.emissivity && card.emissivityTableId == 0)
        return CardProblem{"card.missing-field",
                           "give the emissivity: either " + std::string(CardProblem::FIELD) + " or " +
                               CardProblem::fieldNamed("emissivityTableId"),
                           "emissivity"};
    if (card.emissivity && card.emissivityTableId != 0)
        return CardProblem{"card.bad-value",
                           "give either " + CardProblem::fieldNamed("emissivity") + " or " +
                               std::string(CardProblem::FIELD) + ", not both",
                           "emissivityTableId"};
    if (const auto problem = checkReal(card.environmentTemperature, "environmentTemperature", RealRule::Positive))
        return problem;
    return checkReal(card.stefanBoltzmann, "stefanBoltzmann", RealRule::Positive);
}

void GasBagRadiationFormat::write(const GasBagRadiationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.emissivity)
        writer.real(2, *card.emissivity);
    writer.integerUnlessDefault(3, card.emissivityTableId, 0);
    writer.real(4, card.environmentTemperature);
    writer.real(5, card.stefanBoltzmann);
}

std::uint32_t GasBagRadiationTable::append(const GasBagRadiationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagRadiationTable::store(std::uint32_t row, const GasBagRadiationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagRadiationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagRadiationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagRadiationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "emissivityTableId", ReferenceTarget::DynamicTable, m_cards.edit(row).emissivityTableId);
}

} // namespace oor::model
