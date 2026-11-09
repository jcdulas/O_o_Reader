#include "oor/model/cards/explicit/userShearModel/UserShearModelTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<UserShearModelCard> UserShearModelFormat::read(FieldReader& reader)
{
    UserShearModelCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), USER_SHEAR_MODEL_DEFAULT_ROUTINE_NAME);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserShearModelFormat::check(const UserShearModelCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    return checkRoutineName(card.userRoutineName, USER_SHEAR_MODEL_DEFAULT_ROUTINE_NAME, "userRoutineName");
}

void UserShearModelFormat::write(const UserShearModelCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    writer.text(3, card.userRoutineName);
}

std::uint32_t UserShearModelTable::append(const UserShearModelCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserShearModelTable::store(std::uint32_t row, const UserShearModelCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserShearModelTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserShearModelTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
