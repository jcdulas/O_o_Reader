#include "oor/model/cards/explicit/nobleAbelStateEquation/NobleAbelStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<NobleAbelStateEquationCard> NobleAbelStateEquationFormat::read(FieldReader& reader)
{
    NobleAbelStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.gammaCoefficient = reader.requiredReal(2, "gammaCoefficient");
    card.coVolume = reader.real(3, "coVolume", 0.0);
    card.gasConstant = reader.optionalReal(4, "gasConstant");
    card.specificHeatVolume = reader.optionalReal(5, "specificHeatVolume");
    card.specificHeatPressure = reader.optionalReal(6, "specificHeatPressure");
    if (reader.lastUsedField() > 6)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 6 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NobleAbelStateEquationFormat::check(const NobleAbelStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.gammaCoefficient, "gammaCoefficient", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.coVolume, "coVolume", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkOptionalReal(card.gasConstant, "gasConstant", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.specificHeatVolume, "specificHeatVolume", RealRule::Positive))
        return problem;
    return checkOptionalReal(card.specificHeatPressure, "specificHeatPressure", RealRule::Positive);
}

void NobleAbelStateEquationFormat::write(const NobleAbelStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.gammaCoefficient);
    writer.realUnlessDefault(3, card.coVolume, 0.0);
    if (card.gasConstant)
        writer.real(4, *card.gasConstant);
    if (card.specificHeatVolume)
        writer.real(5, *card.specificHeatVolume);
    if (card.specificHeatPressure)
        writer.real(6, *card.specificHeatPressure);
}

std::uint32_t NobleAbelStateEquationTable::append(const NobleAbelStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NobleAbelStateEquationTable::store(std::uint32_t row, const NobleAbelStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NobleAbelStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NobleAbelStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
