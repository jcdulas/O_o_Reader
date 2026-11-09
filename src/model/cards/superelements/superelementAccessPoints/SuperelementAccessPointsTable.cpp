#include "oor/model/cards/superelements/superelementAccessPoints/SuperelementAccessPointsTable.hpp"

#include "oor/model/cards/sets/DofLists.hpp"

namespace oor::model
{

std::optional<SuperelementAccessPointsCard> SuperelementAccessPointsFormat::read(FieldReader& reader)
{
    SuperelementAccessPointsCard card;
    const int last = reader.lastUsedField();
    // The points follow each other from field 1, over as many lines as the card has; a blank field is padding.
    for (int field = 1; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.nodeIds.push_back(reader.requiredInteger(field, "nodeId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementAccessPointsFormat::check(const SuperelementAccessPointsCard& card)
{
    if (card.nodeIds.empty())
        return CardProblem{"card.missing-field", "the card needs at least one grid point", "nodeId1"};
    for (const base::EntityId nodeId : card.nodeIds)
    {
        if (nodeId <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "nodeId1"};
    }
    return std::nullopt;
}

void SuperelementAccessPointsFormat::write(const SuperelementAccessPointsCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const base::EntityId nodeId : card.nodeIds)
        writer.integer(field++, nodeId);
}

std::uint32_t SuperelementAccessPointsTable::append(const SuperelementAccessPointsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_nodeIds.push(card.nodeIds);
    return row;
}

SuperelementAccessPointsCard SuperelementAccessPointsTable::card(std::uint32_t row) const
{
    const auto nodeIds = m_nodeIds.get(row);
    return SuperelementAccessPointsCard{std::vector<base::EntityId>(nodeIds.begin(), nodeIds.end())};
}

void SuperelementAccessPointsTable::store(std::uint32_t row, const SuperelementAccessPointsCard& card)
{
    m_nodeIds.set(row, card.nodeIds);
}

void SuperelementAccessPointsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_nodeIds.keepRows(rows);
}

void SuperelementAccessPointsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_nodeIds, row, visitor, "nodeId", ReferenceTarget::Node);
}

} // namespace oor::model
