#include "oor/model/cards/explicit/userFailureModel/UserFailureModelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<UserFailureModelCard> UserFailureModelFormat::read(FieldReader& reader)
{
    UserFailureModelCard card;
    card.id = reader.requiredInteger(1, "id");
    if (reader.isBlank(2))
        reader.error("card.missing-field",
                     std::string("groupName") + " is required: the group name of the CONNECT SERVICE statement",
                     "groupName");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), USER_FAILURE_MODEL_DEFAULT_ROUTINE_NAME);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserFailureModelFormat::check(const UserFailureModelCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    return checkRoutineName(card.userRoutineName, USER_FAILURE_MODEL_DEFAULT_ROUTINE_NAME, "userRoutineName");
}

void UserFailureModelFormat::write(const UserFailureModelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    writer.textUnlessDefault(3, card.userRoutineName, USER_FAILURE_MODEL_DEFAULT_ROUTINE_NAME);
}

std::uint32_t UserFailureModelTable::append(const UserFailureModelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserFailureModelTable::store(std::uint32_t row, const UserFailureModelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserFailureModelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserFailureModelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
