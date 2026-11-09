#include "oor/model/cards/explicit/sphereShape/SphereShapeTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<SphereShapeCard> SphereShapeFormat::read(FieldReader& reader)
{
    SphereShapeCard card;
    card.id = reader.requiredInteger(1, "id");
    card.centerX = reader.real(2, "centerX", 0.0);
    card.centerY = reader.real(3, "centerY", 0.0);
    card.centerZ = reader.real(4, "centerZ", 0.0);
    card.radius = reader.requiredReal(5, "radius");
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SphereShapeFormat::check(const SphereShapeCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.centerX, "centerX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerY, "centerY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.centerZ, "centerZ", RealRule::Finite))
        return problem;
    return checkReal(card.radius, "radius", RealRule::Positive);
}

void SphereShapeFormat::write(const SphereShapeCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.centerX, 0.0);
    writer.realUnlessDefault(3, card.centerY, 0.0);
    writer.realUnlessDefault(4, card.centerZ, 0.0);
    writer.real(5, card.radius);
}

std::uint32_t SphereShapeTable::append(const SphereShapeCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SphereShapeTable::store(std::uint32_t row, const SphereShapeCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SphereShapeTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SphereShapeTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
