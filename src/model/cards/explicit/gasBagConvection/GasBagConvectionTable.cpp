#include "oor/model/cards/explicit/gasBagConvection/GasBagConvectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<GasBagConvectionCard> GasBagConvectionFormat::read(FieldReader& reader)
{
    GasBagConvectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficient = reader.optionalReal(2, "coefficient");
    card.coefficientTableId = reader.integer(3, "coefficientTableId", 0);
    card.environmentTemperature = reader.requiredReal(4, "environmentTemperature");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasBagConvectionFormat::check(const GasBagConvectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalReal(card.coefficient, "coefficient", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalId(card.coefficientTableId, "coefficientTableId"))
        return problem;
    // The coefficient is the constant or the table, one of the two.
    if (!card.coefficient && card.coefficientTableId == 0)
        return CardProblem{"card.missing-field",
                           "give the coefficient: either " + std::string(CardProblem::FIELD) + " or " +
                               CardProblem::fieldNamed("coefficientTableId"),
                           "coefficient"};
    if (card.coefficient && card.coefficientTableId != 0)
        return CardProblem{"card.bad-value",
                           "give either " + CardProblem::fieldNamed("coefficient") + " or " +
                               std::string(CardProblem::FIELD) + ", not both",
                           "coefficientTableId"};
    return checkReal(card.environmentTemperature, "environmentTemperature", RealRule::Positive);
}

void GasBagConvectionFormat::write(const GasBagConvectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.coefficient)
        writer.real(2, *card.coefficient);
    writer.integerUnlessDefault(3, card.coefficientTableId, 0);
    writer.real(4, card.environmentTemperature);
}

std::uint32_t GasBagConvectionTable::append(const GasBagConvectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GasBagConvectionTable::store(std::uint32_t row, const GasBagConvectionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GasBagConvectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GasBagConvectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GasBagConvectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "coefficientTableId", ReferenceTarget::DynamicTable, m_cards.edit(row).coefficientTableId);
}

} // namespace oor::model
