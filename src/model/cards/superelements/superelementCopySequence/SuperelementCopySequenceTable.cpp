#include "oor/model/cards/superelements/superelementCopySequence/SuperelementCopySequenceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

} // namespace

std::optional<SuperelementCopySequenceCard> SuperelementCopySequenceFormat::read(FieldReader& reader)
{
    SuperelementCopySequenceCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.primarySuperelementId = reader.requiredInteger(2, "primarySuperelementId");
    const int last = reader.lastUsedField();
    for (int field = FIRST_POINT_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.points.push_back(reader.requiredInteger(field, "node"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementCopySequenceFormat::check(const SuperelementCopySequenceCard& card)
{
    if (const auto problem = checkRequiredId(card.superelementId, "superelementId"))
        return problem;
    if (const auto problem = checkSuperelementId(card.primarySuperelementId, "primarySuperelementId", false))
        return problem;
    if (card.points.empty())
        return CardProblem{"card.missing-field",
                           "the card needs the exterior points " + std::string(CardProblem::FIELD), "nodes"};
    for (const base::EntityId point : card.points)
    {
        if (const auto problem = checkRequiredId(point, "node"))
            return problem;
    }
    return std::nullopt;
}

void SuperelementCopySequenceFormat::write(const SuperelementCopySequenceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.integer(2, card.primarySuperelementId);
    int field = FIRST_POINT_FIELD;
    for (const base::EntityId point : card.points)
        writer.integer(field++, point);
}

std::uint32_t SuperelementCopySequenceTable::append(const SuperelementCopySequenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_cards.push(card);
    return row;
}

void SuperelementCopySequenceTable::store(std::uint32_t row, const SuperelementCopySequenceCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_cards.set(row, card);
}

void SuperelementCopySequenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementCopySequenceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementCopySequenceCard& card = m_cards.edit(row);
    visitSuperelement(visitor, "primarySuperelementId", card.primarySuperelementId);
    SuperelementScope scope(visitor, card.primarySuperelementId);
    for (base::EntityId& point : card.points)
        visitId(scope, "node", ReferenceTarget::Node, point);
}

} // namespace oor::model
