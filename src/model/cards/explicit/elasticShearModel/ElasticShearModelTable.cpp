#include "oor/model/cards/explicit/elasticShearModel/ElasticShearModelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<ElasticShearModelCard> ElasticShearModelFormat::read(FieldReader& reader)
{
    ElasticShearModelCard card;
    card.id = reader.requiredInteger(1, "id");
    card.shearModulus = reader.real(2, "shearModulus", 0.0);
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ElasticShearModelFormat::check(const ElasticShearModelCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkReal(card.shearModulus, "shearModulus", RealRule::Finite);
}

void ElasticShearModelFormat::write(const ElasticShearModelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.shearModulus, 0.0);
}

std::uint32_t ElasticShearModelTable::append(const ElasticShearModelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ElasticShearModelTable::store(std::uint32_t row, const ElasticShearModelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ElasticShearModelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ElasticShearModelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
