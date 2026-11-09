#include "oor/model/cards/solution_control/rotorAxisym/RotorAxisymTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The ranges go up (the format asks INC > 0), and BY is allowed.
constexpr ContactRunRules RULES{.allowBy = true, .allowDescending = false};

constexpr int FIRST_ID_FIELD = 3;

// The number of IDs the list stands for, without building them: a range may cover a million grid points.
std::int64_t countOf(const std::vector<ContactRun>& runs)
{
    std::int64_t total = 0;
    for (const ContactRun& run : runs)
        total += run.isRange() ? (static_cast<std::int64_t>(run.last) - run.first) / run.increment + 1 : 1;
    return total;
}

} // namespace

std::optional<RotorAxisymCard> RotorAxisymFormat::read(FieldReader& reader)
{
    RotorAxisymCard card;
    card.id = reader.requiredInteger(1, "id");
    card.listType = readRequiredKeyword<RotorAxisymListType>(reader, 2, "listType", ROTOR_AXISYM_LIST_WORDS);
    card.ids = readContactRuns(reader, FIRST_ID_FIELD, reader.lastUsedField(), "nodeIds", RULES);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorAxisymFormat::check(const RotorAxisymCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.ids.empty())
        return CardProblem{"card.missing-field", "the card needs at least one ID", "nodeIds"};
    if (const auto problem = checkContactRuns(card.ids, "nodeIds", RULES))
        return problem;
    if (card.listType == RotorAxisymListType::Grid && countOf(card.ids) < 2)
        return CardProblem{"card.bad-value", "a GRID list needs at least two grid points (the axis of the rotor)",
                           "nodeIds"};
    return std::nullopt;
}

void RotorAxisymFormat::write(const RotorAxisymCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, ROTOR_AXISYM_LIST_WORDS, card.listType);
    writeContactRuns(writer, FIRST_ID_FIELD, card.ids);
}

std::uint32_t RotorAxisymTable::append(const RotorAxisymCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_listType.push(static_cast<std::uint8_t>(card.listType));
    m_cards.push(card);
    return row;
}

void RotorAxisymTable::store(std::uint32_t row, const RotorAxisymCard& card)
{
    m_id.set(row, card.id);
    m_listType.set(row, static_cast<std::uint8_t>(card.listType));
    m_cards.set(row, card);
}

void RotorAxisymTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_listType.keepRows(rows);
    m_cards.keepRows(rows);
}

void RotorAxisymTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RotorAxisymTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RotorAxisymCard& card = m_cards.edit(row);
    switch (card.listType)
    {
    case RotorAxisymListType::Element:
        visitContactRuns(card.ids, visitor, "nodeIds", ReferenceTarget::Element, ReferenceTarget::ElementInRange);
        break;
    case RotorAxisymListType::Property:
        visitContactRuns(card.ids, visitor, "nodeIds", ReferenceTarget::Property, ReferenceTarget::PropertyInRange);
        break;
    case RotorAxisymListType::Grid:
        visitContactRuns(card.ids, visitor, "nodeIds", ReferenceTarget::Node, ReferenceTarget::NodeInRange);
        break;
    }
}

} // namespace oor::model
