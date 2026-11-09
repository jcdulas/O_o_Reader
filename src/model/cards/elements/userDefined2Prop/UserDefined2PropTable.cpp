#include "oor/model/cards/elements/userDefined2Prop/UserDefined2PropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LIST_FIELD = 3; // A1: the fields before it are PID and MID

} // namespace

std::optional<UserDefined2PropCard> UserDefined2PropFormat::read(FieldReader& reader)
{
    UserDefined2PropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.fields = readDummyFields(reader, FIRST_LIST_FIELD, "propertyFields");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserDefined2PropFormat::check(const UserDefined2PropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.fields.size() > UserDefined2PropCard::MOST_ADDITIONAL_FIELDS)
        return CardProblem{"card.bad-value", "the card has at most 24 additional fields", "propertyFields"};
    return checkDummyFields(card.fields, "propertyFields");
}

void UserDefined2PropFormat::write(const UserDefined2PropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writeDummyFields(writer, FIRST_LIST_FIELD, card.fields);
}

std::uint32_t UserDefined2PropTable::append(const UserDefined2PropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_fields.push(card.fields);
    return row;
}

UserDefined2PropCard UserDefined2PropTable::card(std::uint32_t row) const
{
    return UserDefined2PropCard{m_id.get(row), m_matId.get(row), m_fields.get(row)};
}

void UserDefined2PropTable::store(std::uint32_t row, const UserDefined2PropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_fields.set(row, card.fields);
}

void UserDefined2PropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_fields.keepRows(rows);
}

void UserDefined2PropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The additional fields are the user's own numbers: they name no card.
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
