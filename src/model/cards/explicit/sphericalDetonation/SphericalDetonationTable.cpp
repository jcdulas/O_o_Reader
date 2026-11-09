#include "oor/model/cards/explicit/sphericalDetonation/SphericalDetonationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<SphericalDetonationCard> SphericalDetonationFormat::read(FieldReader& reader)
{
    SphericalDetonationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.pointX = reader.real(3, "pointX", 0.0);
    card.pointY = reader.real(4, "pointY", 0.0);
    card.pointZ = reader.real(5, "pointZ", 0.0);
    card.detonationSpeed = reader.real(6, "detonationSpeed", 0.0);
    card.detonationTime = reader.real(7, "detonationTime", 0.0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SphericalDetonationFormat::check(const SphericalDetonationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.pointX, "pointX", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.pointY, "pointY", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.pointZ, "pointZ", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.detonationSpeed, "detonationSpeed", RealRule::NonNegative))
        return problem;
    return checkReal(card.detonationTime, "detonationTime", RealRule::NonNegative);
}

void SphericalDetonationFormat::write(const SphericalDetonationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.realUnlessDefault(3, card.pointX, 0.0);
    writer.realUnlessDefault(4, card.pointY, 0.0);
    writer.realUnlessDefault(5, card.pointZ, 0.0);
    writer.realUnlessDefault(6, card.detonationSpeed, 0.0);
    writer.realUnlessDefault(7, card.detonationTime, 0.0);
}

std::uint32_t SphericalDetonationTable::append(const SphericalDetonationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SphericalDetonationTable::store(std::uint32_t row, const SphericalDetonationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SphericalDetonationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SphericalDetonationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SphericalDetonationTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
