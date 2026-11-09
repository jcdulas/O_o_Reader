#include "oor/model/cards/loads/timeTemperatureLoad/TimeTemperatureLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3; // TID

} // namespace

std::optional<TimeTemperatureLoadCard> TimeTemperatureLoadFormat::read(FieldReader& reader)
{
    TimeTemperatureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupId = reader.requiredInteger(2, "groupId");
    card.tableId = reader.requiredInteger(3, "tableId");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("tableId"), "tableId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TimeTemperatureLoadFormat::check(const TimeTemperatureLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // A positive TMPSET number, or -1 for every grid point: any other number is not a group.
    if (card.groupId <= 0 && card.groupId != TimeTemperatureLoadCard::ALL_GRID_POINTS)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) + " must be a positive integer, or -1 for every grid point",
                           "groupId"};
    return checkRequiredId(card.tableId, "tableId");
}

void TimeTemperatureLoadFormat::write(const TimeTemperatureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.groupId);
    writer.integer(3, card.tableId);
}

std::uint32_t TimeTemperatureLoadTable::append(const TimeTemperatureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_groupId.push(card.groupId);
    m_tableId.push(card.tableId);
    return row;
}

TimeTemperatureLoadCard TimeTemperatureLoadTable::card(std::uint32_t row) const
{
    TimeTemperatureLoadCard card;
    card.id = m_id.get(row);
    card.groupId = m_groupId.get(row);
    card.tableId = m_tableId.get(row);
    return card;
}

void TimeTemperatureLoadTable::store(std::uint32_t row, const TimeTemperatureLoadCard& card)
{
    m_id.set(row, card.id);
    m_groupId.set(row, card.groupId);
    m_tableId.set(row, card.tableId);
}

void TimeTemperatureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_groupId.keepRows(rows);
    m_tableId.keepRows(rows);
}

void TimeTemperatureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // GROUP_ID -1 is every grid point in one group: it names no TMPSET, and a renumbering leaves it as it is.
    if (const base::EntityId groupId = m_groupId.get(row); groupId > 0)
        m_groupId.set(row, visitor.visit("groupId", ReferenceTarget::TemperatureGroup, groupId));
    if (const base::EntityId tableId = m_tableId.get(row); tableId > 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::DynamicTable, tableId));
}

} // namespace oor::model
