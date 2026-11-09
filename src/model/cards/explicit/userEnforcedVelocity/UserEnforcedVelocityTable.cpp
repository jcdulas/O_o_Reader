#include "oor/model/cards/explicit/userEnforcedVelocity/UserEnforcedVelocityTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_NODE_FIELD = 9;
constexpr ContactRunRules RULES{true, false}; // THRU and BY are allowed; a range goes upwards

} // namespace

std::optional<UserEnforcedVelocityCard> UserEnforcedVelocityFormat::read(FieldReader& reader)
{
    UserEnforcedVelocityCard card;
    card.setId = reader.requiredInteger(1, "setId");
    card.groupName = std::string(reader.text(2));
    card.userRoutineName = routineNameOf(reader.text(3), UserEnforcedVelocityCard::DEFAULT_ROUTINE_NAME);
    requireBlankFields(reader, {4, 5, 6, 7, 8}, reader.lastUsedField());
    card.grids = readContactRuns(reader, FIRST_NODE_FIELD, reader.lastUsedField(), "node", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserEnforcedVelocityFormat::check(const UserEnforcedVelocityCard& card)
{
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkNameWord(card.groupName, "groupName"))
        return problem;
    if (const auto problem =
            checkRoutineName(card.userRoutineName, UserEnforcedVelocityCard::DEFAULT_ROUTINE_NAME, "userRoutineName"))
        return problem;
    if (card.grids.empty())
        return CardProblem{"card.missing-field", "the card needs grid points (" + std::string(CardProblem::FIELD) + ")",
                           "node"};
    return checkContactRuns(card.grids, "node", RULES);
}

void UserEnforcedVelocityFormat::write(const UserEnforcedVelocityCard& card, FieldWriter& writer)
{
    writer.integer(1, card.setId);
    writer.text(2, card.groupName);
    writer.textUnlessDefault(3, card.userRoutineName, UserEnforcedVelocityCard::DEFAULT_ROUTINE_NAME);
    writeContactRuns(writer, FIRST_NODE_FIELD, card.grids);
}

std::uint32_t UserEnforcedVelocityTable::append(const UserEnforcedVelocityCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void UserEnforcedVelocityTable::store(std::uint32_t row, const UserEnforcedVelocityCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void UserEnforcedVelocityTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserEnforcedVelocityTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void UserEnforcedVelocityTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactRuns(m_cards.edit(row).grids, visitor, "node", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
