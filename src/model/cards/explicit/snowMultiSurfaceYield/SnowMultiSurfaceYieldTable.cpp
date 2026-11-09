#include "oor/model/cards/explicit/snowMultiSurfaceYield/SnowMultiSurfaceYieldTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// A real that the card needs only while the yield surface is not the Drucker-Prager one (CC greater than zero): with a
// CC of zero the solver ignores it, so a blank is stored as zero.
double readSurfaceParameter(FieldReader& reader, int field, const char* name, double cc)
{
    if (cc > 0.0)
        return reader.requiredReal(field, name);
    return reader.real(field, name, 0.0);
}

} // namespace

std::optional<SnowMultiSurfaceYieldCard> SnowMultiSurfaceYieldFormat::read(FieldReader& reader)
{
    SnowMultiSurfaceYieldCard card;
    card.id = reader.requiredInteger(1, "id");
    card.coefficientKc = reader.requiredReal(2, "coefficientKc");
    card.coefficientT = reader.requiredReal(3, "coefficientT");
    card.coefficientCc = reader.requiredReal(4, "coefficientCc");
    card.ac = readSurfaceParameter(reader, 5, "coefficientAc", card.coefficientCc);
    card.bc = readSurfaceParameter(reader, 6, "coefficientBc", card.coefficientCc);
    card.coefficientFc = reader.real(7, "coefficientFc", SNOW_MULTI_SURFACE_DEFAULT_COEFFICIENT_FC);
    const std::optional<double> tensileStrength = reader.optionalReal(8, "coefficientFtu");
    card.coefficientFtu = tensileStrength ? *tensileStrength : card.coefficientT / 3.0;
    card.initialAlpha = readSurfaceParameter(reader, 9, "initialAlpha", card.coefficientCc);
    card.coefficientDs = reader.real(10, "coefficientDs", 0.0);
    if (reader.lastUsedField() > 10)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 10 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SnowMultiSurfaceYieldFormat::check(const SnowMultiSurfaceYieldCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.coefficientKc, "coefficientKc", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientT, "coefficientT", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.coefficientCc, "coefficientCc", RealRule::NonNegative))
        return problem;
    if (card.coefficientCc > 0.0)
    {
        if (const auto problem = checkReal(card.ac, "coefficientAc", RealRule::Positive))
            return problem;
        if (const auto problem = checkReal(card.bc, "coefficientBc", RealRule::Positive))
            return problem;
    }
    else
    {
        if (const auto problem = checkReal(card.ac, "coefficientAc", RealRule::Finite))
            return problem;
        if (const auto problem = checkReal(card.bc, "coefficientBc", RealRule::Finite))
            return problem;
    }
    if (!(card.coefficientFc > 0.0 && card.coefficientFc < 1.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be greater than zero and less than one",
                           "coefficientFc"};
    if (const auto problem = checkReal(card.coefficientFtu, "coefficientFtu", RealRule::Positive))
        return problem;
    if (card.coefficientCc > 0.0 && !(card.initialAlpha < 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be less than zero",
                           "initialAlpha"};
    if (const auto problem = checkReal(card.initialAlpha, "initialAlpha", RealRule::Finite))
        return problem;
    return checkReal(card.coefficientDs, "coefficientDs", RealRule::NonNegative);
}

void SnowMultiSurfaceYieldFormat::write(const SnowMultiSurfaceYieldCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.coefficientKc);
    writer.real(3, card.coefficientT);
    writer.real(4, card.coefficientCc);
    writer.real(5, card.ac);
    writer.real(6, card.bc);
    writer.realUnlessDefault(7, card.coefficientFc, SNOW_MULTI_SURFACE_DEFAULT_COEFFICIENT_FC);
    writer.real(8, card.coefficientFtu); // its default is T / 3: stored resolved, so always written
    writer.real(9, card.initialAlpha);
    writer.realUnlessDefault(10, card.coefficientDs, 0.0);
}

std::uint32_t SnowMultiSurfaceYieldTable::append(const SnowMultiSurfaceYieldCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SnowMultiSurfaceYieldTable::store(std::uint32_t row, const SnowMultiSurfaceYieldCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SnowMultiSurfaceYieldTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SnowMultiSurfaceYieldTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
