#include "oor/model/cards/sets/userDofSet/UserDofSetTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

// SNAME in field 1, then ID1 C1 ID2 C2 ID3 C3 on the first line (field 8 is blank), then the same pairs on the
// continuation lines.
constexpr DofPairLayout LAYOUT{2, 3};

} // namespace

std::optional<UserDofSetCard> UserDofSetFormat::read(FieldReader& reader)
{
    UserDofSetCard card;
    card.setName = std::string(reader.text(1));
    if (card.setName.empty())
    {
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " card needs a set name", "setName");
        return card;
    }
    card.dofs = readDofPairs(reader, LAYOUT);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserDofSetFormat::check(const UserDofSetCard& card)
{
    if (const auto problem = checkNameWord(card.setName, "setName"))
        return problem;
    return checkDofPairs(card.dofs);
}

void UserDofSetFormat::write(const UserDofSetCard& card, FieldWriter& writer)
{
    writer.text(1, card.setName);
    writeDofPairs(writer, LAYOUT, card.dofs);
}

std::uint32_t UserDofSetTable::append(const UserDofSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setName.push(card.setName);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

UserDofSetCard UserDofSetTable::card(std::uint32_t row) const
{
    UserDofSetCard card;
    card.setName = m_setName.get(row);
    card.dofs = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void UserDofSetTable::store(std::uint32_t row, const UserDofSetCard& card)
{
    m_setName.set(row, card.setName);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.dofs, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void UserDofSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setName.keepRows(rows);
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void UserDofSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
