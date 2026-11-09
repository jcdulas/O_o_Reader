#include "oor/model/cards/loads/metadataGroup/MetadataGroupTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_ENTITY_FIELD = 4;

} // namespace

std::optional<MetadataGroupCard> MetadataGroupFormat::read(FieldReader& reader)
{
    MetadataGroupCard card;
    card.groupName = std::string(reader.text(1));
    card.id = reader.requiredInteger(2, "id");
    card.entityName = std::string(reader.text(3));
    const int last = reader.lastUsedField();
    for (int field = FIRST_ENTITY_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.entities.push_back(reader.requiredInteger(field, "entityId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MetadataGroupFormat::check(const MetadataGroupCard& card)
{
    if (const auto problem = checkNameWord(card.groupName, "metadataGroupName"))
        return problem;
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the group ID must be a positive integer", "id"};
    if (const auto problem = checkNameWord(card.entityName, "entityName"))
        return problem;
    if (card.entities.empty())
        return CardProblem{"card.missing-field", "the card needs at least one entity ID", "entityId1"};
    for (const base::EntityId entity : card.entities)
    {
        if (entity <= 0)
            return CardProblem{"card.bad-id", "an entity ID must be a positive integer", "entityId1"};
    }
    return std::nullopt;
}

void MetadataGroupFormat::write(const MetadataGroupCard& card, FieldWriter& writer)
{
    writer.text(1, card.groupName);
    writer.integer(2, card.id);
    writer.text(3, card.entityName);
    int field = FIRST_ENTITY_FIELD;
    for (const base::EntityId entity : card.entities)
        writer.integer(field++, entity);
}

std::uint32_t MetadataGroupTable::append(const MetadataGroupCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_groupName.push(card.groupName);
    m_id.push(card.id);
    m_entityName.push(card.entityName);
    m_entities.push(card.entities);
    return row;
}

MetadataGroupCard MetadataGroupTable::card(std::uint32_t row) const
{
    MetadataGroupCard card;
    card.groupName = m_groupName.get(row);
    card.id = m_id.get(row);
    card.entityName = m_entityName.get(row);
    const auto entities = m_entities.get(row);
    card.entities.assign(entities.begin(), entities.end());
    return card;
}

void MetadataGroupTable::store(std::uint32_t row, const MetadataGroupCard& card)
{
    m_groupName.set(row, card.groupName);
    m_id.set(row, card.id);
    m_entityName.set(row, card.entityName);
    m_entities.set(row, card.entities);
}

void MetadataGroupTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_groupName.keepRows(rows);
    m_id.keepRows(rows);
    m_entityName.keepRows(rows);
    m_entities.keepRows(rows);
}

void MetadataGroupTable::visitReferences(std::uint32_t /*row*/, ReferenceVisitor& /*visitor*/)
{
    // The entities are of the kind NASTNM names (any card type): they are meta data and are not resolved.
}

} // namespace oor::model
