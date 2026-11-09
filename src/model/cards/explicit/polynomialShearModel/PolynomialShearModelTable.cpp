#include "oor/model/cards/explicit/polynomialShearModel/PolynomialShearModelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<PolynomialShearModelCard> PolynomialShearModelFormat::read(FieldReader& reader)
{
    PolynomialShearModelCard card;
    card.id = reader.requiredInteger(1, "id");
    card.shearModulus0 = reader.real(2, "shearModulus0", 0.0);
    card.shearModulus1 = reader.real(3, "shearModulus1", 0.0);
    card.shearModulus2 = reader.real(4, "shearModulus2", 0.0);
    card.shearModulus3 = reader.real(5, "shearModulus3", 0.0);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PolynomialShearModelFormat::check(const PolynomialShearModelCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.shearModulus0, "shearModulus0", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.shearModulus1, "shearModulus1", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.shearModulus2, "shearModulus2", RealRule::Finite))
        return problem;
    return checkReal(card.shearModulus3, "shearModulus3", RealRule::Finite);
}

void PolynomialShearModelFormat::write(const PolynomialShearModelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.shearModulus0, 0.0);
    writer.realUnlessDefault(3, card.shearModulus1, 0.0);
    writer.realUnlessDefault(4, card.shearModulus2, 0.0);
    writer.realUnlessDefault(5, card.shearModulus3, 0.0);
}

std::uint32_t PolynomialShearModelTable::append(const PolynomialShearModelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PolynomialShearModelTable::store(std::uint32_t row, const PolynomialShearModelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PolynomialShearModelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PolynomialShearModelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
