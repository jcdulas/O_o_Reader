#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_SET_FIELD = 2;

} // namespace

std::optional<DesignConstrUnionCard> DesignConstrUnionFormat::read(FieldReader& reader)
{
    DesignConstrUnionCard card;
    card.constraintSetId = reader.requiredInteger(1, "constraintSetId");
    for (int field = FIRST_SET_FIELD; field <= reader.lastUsedField(); ++field)
    {
        if (!reader.isBlank(field))
            card.sets.push_back(reader.requiredInteger(field, "memberSetId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DesignConstrUnionFormat::check(const DesignConstrUnionCard& card)
{
    if (const auto problem = checkRequiredId(card.constraintSetId, "constraintSetId"))
        return problem;
    if (card.sets.empty())
        return CardProblem{"card.missing-field",
                           "at least one constraint set (" + std::string(CardProblem::FIELD) + ") is required",
                           "memberSetId"};
    for (const base::EntityId set : card.sets)
    {
        if (const auto problem = checkRequiredId(set, "memberSetId"))
            return problem;
    }
    return std::nullopt;
}

void DesignConstrUnionFormat::write(const DesignConstrUnionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.constraintSetId);
    int field = FIRST_SET_FIELD;
    for (const base::EntityId set : card.sets)
        writer.integer(field++, set);
}

std::uint32_t DesignConstrUnionTable::append(const DesignConstrUnionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_constraintSetId.push(card.constraintSetId);
    m_cards.push(card);
    return row;
}

void DesignConstrUnionTable::store(std::uint32_t row, const DesignConstrUnionCard& card)
{
    m_constraintSetId.set(row, card.constraintSetId);
    m_cards.set(row, card);
}

void DesignConstrUnionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_constraintSetId.keepRows(rows);
    m_cards.keepRows(rows);
}

void DesignConstrUnionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_constraintSetId.set(row, id);
    m_cards.edit(row).constraintSetId = id;
}

void DesignConstrUnionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (base::EntityId& set : m_cards.edit(row).sets)
        visitId(visitor, "memberSetId", ReferenceTarget::DesignConstraintSet, set);
}

} // namespace oor::model
