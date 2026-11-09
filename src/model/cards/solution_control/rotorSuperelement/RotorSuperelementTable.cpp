#include "oor/model/cards/solution_control/rotorSuperelement/RotorSuperelementTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

std::optional<RotorSuperelementCard> RotorSuperelementFormat::read(FieldReader& reader)
{
    RotorSuperelementCard card;
    card.id = reader.requiredInteger(1, "id");
    card.superelementId = reader.requiredInteger(2, "superelementId");
    card.superelementOption =
        reader.integer(3, "superelementOption", RotorSuperelementCard::DEFAULT_SUPERELEMENT_OPTION);
    requireBlankFields(reader, {}, 3);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RotorSuperelementFormat::check(const RotorSuperelementCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    // The superelement is a positive number: the residual structure (0) has no rotor superelement.
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", false))
        return problem;
    if (card.superelementOption != 1 && card.superelementOption != 2)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 1 (A-set matrices) or 2 (G-set matrices)",
                           "superelementOption"};
    return std::nullopt;
}

void RotorSuperelementFormat::write(const RotorSuperelementCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.superelementId);
    writer.integerUnlessDefault(3, card.superelementOption, RotorSuperelementCard::DEFAULT_SUPERELEMENT_OPTION);
}

void RotorSuperelementTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId superelementId = m_superelementId.get(row);
    visitSuperelement(visitor, "superelementId", superelementId);
    m_superelementId.set(row, superelementId);
}

std::uint32_t RotorSuperelementTable::append(const RotorSuperelementCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_superelementId.push(card.superelementId);
    m_superelementOption.push(card.superelementOption);
    return row;
}

RotorSuperelementCard RotorSuperelementTable::card(std::uint32_t row) const
{
    RotorSuperelementCard card;
    card.id = m_id.get(row);
    card.superelementId = m_superelementId.get(row);
    card.superelementOption = m_superelementOption.get(row);
    return card;
}

void RotorSuperelementTable::store(std::uint32_t row, const RotorSuperelementCard& card)
{
    m_id.set(row, card.id);
    m_superelementId.set(row, card.superelementId);
    m_superelementOption.set(row, card.superelementOption);
}

void RotorSuperelementTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_superelementId.keepRows(rows);
    m_superelementOption.keepRows(rows);
}

} // namespace oor::model
