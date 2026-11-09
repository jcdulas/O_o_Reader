#include "oor/model/cards/explicit/porousUserBoundary/PorousUserBoundaryTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <cctype>

namespace oor::model
{

std::optional<PorousUserBoundaryCard> PorousUserBoundaryFormat::read(FieldReader& reader)
{
    PorousUserBoundaryCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), POROUS_USER_BOUNDARY_DEFAULT_ROUTINE_NAME);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PorousUserBoundaryFormat::check(const PorousUserBoundaryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem = checkNameWord(card.userRoutineName, "userRoutineName"))
        return problem;
    return std::nullopt;
}

void PorousUserBoundaryFormat::write(const PorousUserBoundaryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    writer.text(3, card.userRoutineName);
}

std::uint32_t PorousUserBoundaryTable::append(const PorousUserBoundaryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PorousUserBoundaryTable::store(std::uint32_t row, const PorousUserBoundaryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PorousUserBoundaryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PorousUserBoundaryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
