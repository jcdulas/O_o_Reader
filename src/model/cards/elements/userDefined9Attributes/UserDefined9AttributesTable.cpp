#include "oor/model/cards/elements/userDefined9Attributes/UserDefined9AttributesTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;
constexpr std::int32_t MOST_PROPERTY_FIELDS = 24; // the most additional fields a PDUM9 card has (the format: NP <= 24)

} // namespace

std::optional<UserDefined9AttributesCard> UserDefined9AttributesFormat::read(FieldReader& reader)
{
    UserDefined9AttributesCard card;
    card.nodeCount = reader.requiredInteger(1, "nodeCount");
    card.connectionFieldCount = reader.integer(2, "connectionFieldCount", 0);
    card.propertyFieldCount = reader.integer(3, "propertyFieldCount", 0);
    card.displacementComponentCount = reader.integer(4, "displacementComponentCount", 0);
    card.elementName = std::string(reader.word(5));
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserDefined9AttributesFormat::check(const UserDefined9AttributesCard& card)
{
    if (card.nodeCount <= 0)
        return CardProblem{"card.bad-value", "{field} must be a positive integer", "nodeCount"};
    if (card.connectionFieldCount < 0)
        return CardProblem{"card.bad-value", "{field} must not be negative", "connectionFieldCount"};
    if (card.propertyFieldCount < 0 || card.propertyFieldCount > MOST_PROPERTY_FIELDS)
        return CardProblem{"card.bad-value", "{field} must be an integer from 0 to 24", "propertyFieldCount"};
    if (card.displacementComponentCount != 0 && card.displacementComponentCount != 3 &&
        card.displacementComponentCount != 6)
        return CardProblem{"card.bad-value", "{field} must be 0 (no differential stiffness), 3 or 6",
                           "displacementComponentCount"};
    // The format reference gives ELNM no default and does not say it is required: a blank one leaves the connection and
    // property cards their own names, CDUMi and PDUMi (the library's reading).
    if (card.elementName.empty())
        return std::nullopt;
    return checkCardTypeWord(card.elementName, "elementName");
}

void UserDefined9AttributesFormat::write(const UserDefined9AttributesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.nodeCount);
    writer.integer(2, card.connectionFieldCount);
    writer.integer(3, card.propertyFieldCount);
    writer.integer(4, card.displacementComponentCount);
    if (!card.elementName.empty())
        writer.text(5, card.elementName);
}

std::uint32_t UserDefined9AttributesTable::append(const UserDefined9AttributesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_nodeCount.push(card.nodeCount);
    m_connectionFieldCount.push(card.connectionFieldCount);
    m_propertyFieldCount.push(card.propertyFieldCount);
    m_displacementComponentCount.push(card.displacementComponentCount);
    m_elementName.push(card.elementName);
    return row;
}

UserDefined9AttributesCard UserDefined9AttributesTable::card(std::uint32_t row) const
{
    return UserDefined9AttributesCard{m_nodeCount.get(row), m_connectionFieldCount.get(row), m_propertyFieldCount.get(row),
                                   m_displacementComponentCount.get(row), m_elementName.get(row)};
}

void UserDefined9AttributesTable::store(std::uint32_t row, const UserDefined9AttributesCard& card)
{
    m_nodeCount.set(row, card.nodeCount);
    m_connectionFieldCount.set(row, card.connectionFieldCount);
    m_propertyFieldCount.set(row, card.propertyFieldCount);
    m_displacementComponentCount.set(row, card.displacementComponentCount);
    m_elementName.set(row, card.elementName);
}

void UserDefined9AttributesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_nodeCount.keepRows(rows);
    m_connectionFieldCount.keepRows(rows);
    m_propertyFieldCount.keepRows(rows);
    m_displacementComponentCount.keepRows(rows);
    m_elementName.keepRows(rows);
}

} // namespace oor::model
