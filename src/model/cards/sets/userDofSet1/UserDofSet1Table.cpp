#include "oor/model/cards/sets/userDofSet1/UserDofSet1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<UserDofSet1Card> UserDofSet1Format::read(FieldReader& reader)
{
    UserDofSet1Card card;
    card.setName = std::string(reader.text(1));
    if (card.setName.empty())
    {
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " card needs a set name", "setName");
        return card;
    }
    card.components = base::sortedDigits(reader.integer(2, "components", 0));
    card.nodes = readIdRuns(reader, FIRST_POINT_FIELD, "nodes");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserDofSet1Format::check(const UserDofSet1Card& card)
{
    if (const auto problem = checkNameWord(card.setName, "setName"))
        return problem;
    if (!base::isComponentSet(card.components))
        return CardProblem{"card.bad-value", "C must be 0 or a list of the digits 1 to 6, each at most once",
                           "components"};
    return checkIdRuns(card.nodes, "nodes");
}

void UserDofSet1Format::write(const UserDofSet1Card& card, FieldWriter& writer)
{
    writer.text(1, card.setName);
    writer.integerUnlessDefault(2, card.components, 0);
    writeIdRuns(writer, FIRST_POINT_FIELD, card.nodes, writer.compressesRanges());
}

std::uint32_t UserDofSet1Table::append(const UserDofSet1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setName.push(card.setName);
    m_components.push(card.components);
    m_runs.push(toPairs(card.nodes));
    return row;
}

UserDofSet1Card UserDofSet1Table::card(std::uint32_t row) const
{
    UserDofSet1Card card;
    card.setName = m_setName.get(row);
    card.components = m_components.get(row);
    card.nodes = toIdRuns(m_runs.get(row));
    return card;
}

void UserDofSet1Table::store(std::uint32_t row, const UserDofSet1Card& card)
{
    m_setName.set(row, card.setName);
    m_components.set(row, card.components);
    m_runs.set(row, toPairs(card.nodes));
}

void UserDofSet1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setName.keepRows(rows);
    m_components.keepRows(rows);
    m_runs.keepRows(rows);
}

void UserDofSet1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdRuns(m_runs, row, visitor, "nodes", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
}

} // namespace oor::model
