#include "oor/model/cards/explicit/userStateEquation/UserStateEquationTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<UserStateEquationCard> UserStateEquationFormat::read(FieldReader& reader)
{
    UserStateEquationCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), USER_STATE_EQUATION_DEFAULT_ROUTINE_NAME);
    card.viscosity = reader.optionalReal(4, "viscosity");
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserStateEquationFormat::check(const UserStateEquationCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem =
            checkRoutineName(card.userRoutineName, USER_STATE_EQUATION_DEFAULT_ROUTINE_NAME, "userRoutineName"))
        return problem;
    return checkOptionalReal(card.viscosity, "viscosity", RealRule::NonNegative);
}

void UserStateEquationFormat::write(const UserStateEquationCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    writer.text(3, card.userRoutineName);
    if (card.viscosity)
        writer.real(4, *card.viscosity);
}

std::uint32_t UserStateEquationTable::append(const UserStateEquationCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserStateEquationTable::store(std::uint32_t row, const UserStateEquationCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserStateEquationTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserStateEquationTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
