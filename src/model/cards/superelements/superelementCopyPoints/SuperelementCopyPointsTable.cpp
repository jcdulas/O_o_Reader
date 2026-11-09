#include "oor/model/cards/superelements/superelementCopyPoints/SuperelementCopyPointsTable.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cstdlib>

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;

// The codes the format reference gives for the components a mirror reverses.
bool isReversalCode(base::EntityId code) noexcept
{
    constexpr std::array<base::EntityId, 8> CODES{0, 1, 2, 3, 12, 23, 31, 123};
    for (const base::EntityId known : CODES)
    {
        if (known == code)
            return true;
    }
    return false;
}

} // namespace

std::optional<SuperelementCopyPointsCard> SuperelementCopyPointsFormat::read(FieldReader& reader)
{
    SuperelementCopyPointsCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.primarySuperelementId = reader.integer(2, "primarySuperelementId", 0);
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

std::optional<CardProblem> SuperelementCopyPointsFormat::check(const SuperelementCopyPointsCard& card)
{
    if (const auto problem = checkRequiredId(card.superelementId, "superelementId"))
        return problem;
    if (card.number() == 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must end with the number of the superelement (1 to 9999)",
            "superelementId"};
    if (!isReversalCode(card.reversalCode()))
        return CardProblem{"card.bad-value",
                           "the code before the last four digits of " + std::string(CardProblem::FIELD) +
                               " is 1, 2, 3, 12, 23, 31 or 123",
                           "superelementId"};
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

void SuperelementCopyPointsFormat::write(const SuperelementCopyPointsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writer.integerUnlessDefault(2, card.primarySuperelementId, 0);
    int field = FIRST_POINT_FIELD;
    for (const base::EntityId point : card.points)
        writer.integer(field++, point);
}

std::uint32_t SuperelementCopyPointsTable::append(const SuperelementCopyPointsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_number.push(card.number());
    m_cards.push(card);
    return row;
}

void SuperelementCopyPointsTable::store(std::uint32_t row, const SuperelementCopyPointsCard& card)
{
    m_number.set(row, card.number());
    m_cards.set(row, card);
}

void SuperelementCopyPointsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_number.keepRows(rows);
    m_cards.keepRows(rows);
}

void SuperelementCopyPointsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    if (id <= 0 || id >= SuperelementCopyPointsCard::NUMBER_RANGE)
        throw base::UsageError("the number of a secondary superelement (CSUPER) must be between 1 and 9999, not " +
                               std::to_string(id));
    SuperelementCopyPointsCard& card = m_cards.edit(row);
    card.superelementId = card.reversalCode() * SuperelementCopyPointsCard::NUMBER_RANGE + id;
    m_number.set(row, id);
}

void SuperelementCopyPointsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SuperelementCopyPointsCard& card = m_cards.edit(row);
    if (card.primarySuperelementId != 0)
    {
        const base::EntityId primary = std::abs(card.primarySuperelementId);
        const base::EntityId updated = visitor.visit("primarySuperelementId", ReferenceTarget::Superelement, primary);
        card.primarySuperelementId = card.primarySuperelementId < 0 ? -updated : updated;
    }
    for (base::EntityId& point : card.points)
        visitId(visitor, "node", ReferenceTarget::Node, point);
}

} // namespace oor::model
