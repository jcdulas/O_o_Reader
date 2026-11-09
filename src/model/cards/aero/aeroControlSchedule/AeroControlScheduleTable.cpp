#include "oor/model/cards/aero/aeroControlSchedule/AeroControlScheduleTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<AeroControlScheduleCard> AeroControlScheduleFormat::read(FieldReader& reader)
{
    AeroControlScheduleCard card;
    card.id = reader.requiredInteger(1, "id");
    card.controlSurfaceId = reader.requiredInteger(2, "controlSurfaceId");
    card.angleOfAttackListId = reader.integer(3, "angleOfAttackListId", 0);
    card.machListId = reader.integer(4, "machListId", 0);
    card.scheduleListId = reader.requiredInteger(5, "scheduleListId");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroControlScheduleFormat::check(const AeroControlScheduleCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.controlSurfaceId, "controlSurfaceId"))
        return problem;
    if (const auto problem = checkOptionalId(card.angleOfAttackListId, "angleOfAttackListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.machListId, "machListId"))
        return problem;
    if (const auto problem = checkRequiredId(card.scheduleListId, "scheduleListId"))
        return problem;
    // a schedule needs the angles of attack or the Mach numbers (or both).
    if (card.angleOfAttackListId == 0 && card.machListId == 0)
        return CardProblem{"card.missing-field", "{field} and {field:machListId} cannot both be blank",
                           "angleOfAttackListId"};
    return std::nullopt;
}

void AeroControlScheduleFormat::write(const AeroControlScheduleCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.controlSurfaceId);
    writer.integerUnlessDefault(3, card.angleOfAttackListId, 0);
    writer.integerUnlessDefault(4, card.machListId, 0);
    writer.integer(5, card.scheduleListId);
}

std::uint32_t AeroControlScheduleTable::append(const AeroControlScheduleCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroControlScheduleTable::store(std::uint32_t row, const AeroControlScheduleCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroControlScheduleTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroControlScheduleTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroControlScheduleTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroControlScheduleCard& card = m_cards.edit(row);
    visitId(visitor, "controlSurfaceId", ReferenceTarget::AeroControlSurface, card.controlSurfaceId);
    visitId(visitor, "angleOfAttackListId", ReferenceTarget::AeroFactor, card.angleOfAttackListId);
    visitId(visitor, "machListId", ReferenceTarget::AeroFactor, card.machListId);
    visitId(visitor, "scheduleListId", ReferenceTarget::AeroFactor, card.scheduleListId);
}

} // namespace oor::model
