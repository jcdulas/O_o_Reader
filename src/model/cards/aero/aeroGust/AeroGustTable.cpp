#include "oor/model/cards/aero/aeroGust/AeroGustTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<AeroGustCard> AeroGustFormat::read(FieldReader& reader)
{
    AeroGustCard card;
    card.id = reader.requiredInteger(1, "id");
    card.dynamicLoadId = reader.requiredInteger(2, "dynamicLoadId");
    card.gustScale = reader.requiredReal(3, "gustScale");
    card.gustOriginX = reader.requiredReal(4, "gustOriginX");
    card.velocity = reader.requiredReal(5, "velocity");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroGustFormat::check(const AeroGustCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.dynamicLoadId, "dynamicLoadId"))
        return problem;
    if (const auto problem = checkReal(card.gustScale, "gustScale", RealRule::Finite))
        return problem;
    if (card.gustScale == 0.0)
        return CardProblem{"card.bad-value", "{field} must not be zero", "gustScale"};
    if (const auto problem = checkReal(card.gustOriginX, "gustOriginX", RealRule::Finite))
        return problem;
    return checkReal(card.velocity, "velocity", RealRule::Positive);
}

void AeroGustFormat::write(const AeroGustCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.dynamicLoadId);
    writer.real(3, card.gustScale);
    // X0 is zero as often as not and has no default: it is written even then.
    writer.real(4, card.gustOriginX);
    writer.real(5, card.velocity);
}

std::uint32_t AeroGustTable::append(const AeroGustCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_dynamicLoadId.push(card.dynamicLoadId);
    m_gustScale.push(card.gustScale);
    m_gustOriginX.push(card.gustOriginX);
    m_velocity.push(card.velocity);
    return row;
}

AeroGustCard AeroGustTable::card(std::uint32_t row) const
{
    AeroGustCard card;
    card.id = m_id.get(row);
    card.dynamicLoadId = m_dynamicLoadId.get(row);
    card.gustScale = m_gustScale.get(row);
    card.gustOriginX = m_gustOriginX.get(row);
    card.velocity = m_velocity.get(row);
    return card;
}

void AeroGustTable::store(std::uint32_t row, const AeroGustCard& card)
{
    m_id.set(row, card.id);
    m_dynamicLoadId.set(row, card.dynamicLoadId);
    m_gustScale.set(row, card.gustScale);
    m_gustOriginX.set(row, card.gustOriginX);
    m_velocity.set(row, card.velocity);
}

void AeroGustTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_dynamicLoadId.keepRows(rows);
    m_gustScale.keepRows(rows);
    m_gustOriginX.keepRows(rows);
    m_velocity.keepRows(rows);
}

void AeroGustTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId dynamicLoadId = m_dynamicLoadId.get(row); dynamicLoadId > 0)
        m_dynamicLoadId.set(row, visitor.visit("dynamicLoadId", ReferenceTarget::DynamicLoadSet, dynamicLoadId));
}

} // namespace oor::model
