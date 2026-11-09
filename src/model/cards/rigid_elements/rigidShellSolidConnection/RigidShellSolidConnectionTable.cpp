#include "oor/model/cards/rigid_elements/rigidShellSolidConnection/RigidShellSolidConnectionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr const char* ID_NAMES[RigidShellSolidConnectionCard::ID_COUNT] = {"shellId1", "solidIdA1", "solidIdB1",
                                                                           "shellId2", "solidIdA2", "solidIdB2"};

} // namespace

std::optional<RigidShellSolidConnectionCard> RigidShellSolidConnectionFormat::read(FieldReader& reader)
{
    RigidShellSolidConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    const std::string_view word = reader.text(2);
    if (word.empty())
        card.kind = RigidShellSolidConnectionType::Element;
    else if (const auto type =
                 parseKeyword<RigidShellSolidConnectionType>(RIGID_SHELL_SOLID_CONNECTION_TYPE_WORDS, word))
        card.kind = *type;
    else
        reader.error("card.bad-value",
                     std::string("kind") + " must be " + keywordList(RIGID_SHELL_SOLID_CONNECTION_TYPE_WORDS), "kind");
    for (std::size_t index = 0; index < RigidShellSolidConnectionCard::ID_COUNT; ++index)
        card.ids[index] = reader.integer(3 + static_cast<int>(index), ID_NAMES[index], 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidShellSolidConnectionFormat::check(const RigidShellSolidConnectionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    for (std::size_t index = 0; index < RigidShellSolidConnectionCard::ID_COUNT; ++index)
    {
        if (card.ids[index] < 0)
            return CardProblem{"card.bad-id", "an ID must be blank or a positive integer", ID_NAMES[index]};
    }
    if (card.ids[0] == 0)
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "shellId1"};
    if (card.ids[1] == 0)
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "solidIdA1"};
    if (card.kind == RigidShellSolidConnectionType::Element)
    {
        // The fields that are for TYPE = GRID only must be blank: nothing is dropped silently.
        for (std::size_t index = 2; index < RigidShellSolidConnectionCard::ID_COUNT; ++index)
        {
            if (card.ids[index] != 0)
                return CardProblem{"card.bad-value",
                                   "this field is for " + CardProblem::fieldNamed("kind") + " = " +
                                       CardProblem::cardNamed("Node") + " only",
                                   ID_NAMES[index]};
        }
    }
    return std::nullopt;
}

void RigidShellSolidConnectionFormat::write(const RigidShellSolidConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, RIGID_SHELL_SOLID_CONNECTION_TYPE_WORDS, card.kind,
                              RigidShellSolidConnectionType::Element);
    for (std::size_t index = 0; index < RigidShellSolidConnectionCard::ID_COUNT; ++index)
        writer.integerUnlessDefault(3 + static_cast<int>(index), card.ids[index], 0);
}

std::uint32_t RigidShellSolidConnectionTable::append(const RigidShellSolidConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(static_cast<std::uint8_t>(card.kind));
    m_ids.push(card.ids);
    return row;
}

RigidShellSolidConnectionCard RigidShellSolidConnectionTable::card(std::uint32_t row) const
{
    return RigidShellSolidConnectionCard{m_id.get(row), static_cast<RigidShellSolidConnectionType>(m_type.get(row)),
                                         m_ids.get(row)};
}

void RigidShellSolidConnectionTable::store(std::uint32_t row, const RigidShellSolidConnectionCard& card)
{
    m_id.set(row, card.id);
    m_type.set(row, static_cast<std::uint8_t>(card.kind));
    m_ids.set(row, card.ids);
}

void RigidShellSolidConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_ids.keepRows(rows);
}

void RigidShellSolidConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, RigidShellSolidConnectionCard::ID_COUNT> ids = m_ids.get(row);
    const bool elements =
        static_cast<RigidShellSolidConnectionType>(m_type.get(row)) == RigidShellSolidConnectionType::Element;
    for (std::size_t index = 0; index < RigidShellSolidConnectionCard::ID_COUNT; ++index)
    {
        if (ids[index] == 0)
            continue;
        ids[index] =
            visitor.visit(ID_NAMES[index], elements ? ReferenceTarget::Element : ReferenceTarget::Node, ids[index]);
    }
    m_ids.set(row, ids);
}

} // namespace oor::model
