#include "oor/model/cards/constraints/multiPointConstrUnion/MultiPointConstrUnionTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_SET_FIELD = 2;

} // namespace

std::optional<MultiPointConstrUnionCard> MultiPointConstrUnionFormat::read(FieldReader& reader)
{
    MultiPointConstrUnionCard card;
    card.id = reader.requiredInteger(1, "id");
    // The set IDs from field 2 on. Blank fields (the padding of the continuation lines) are skipped.
    const int last = reader.lastUsedField();
    for (int field = FIRST_SET_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.sets.push_back(reader.requiredInteger(field, "memberSetId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MultiPointConstrUnionFormat::check(const MultiPointConstrUnionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.sets.empty())
        return CardProblem{"card.missing-field", "the card needs at least one constraint set", "memberSetId1"};
    for (const base::EntityId set : card.sets)
    {
        if (set <= 0)
            return CardProblem{"card.bad-id", "a constraint set ID must be a positive integer", "memberSetId1"};
        if (set == card.id)
            return CardProblem{"card.bad-value", "a union of sets cannot refer to its own set ID", "memberSetId1"};
    }
    return std::nullopt;
}

void MultiPointConstrUnionFormat::write(const MultiPointConstrUnionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_SET_FIELD;
    for (const base::EntityId set : card.sets)
        writer.integer(field++, set);
}

std::uint32_t MultiPointConstrUnionTable::append(const MultiPointConstrUnionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_sets.push(card.sets);
    return row;
}

MultiPointConstrUnionCard MultiPointConstrUnionTable::card(std::uint32_t row) const
{
    MultiPointConstrUnionCard card;
    card.id = m_id.get(row);
    const auto sets = m_sets.get(row);
    card.sets.assign(sets.begin(), sets.end());
    return card;
}

void MultiPointConstrUnionTable::store(std::uint32_t row, const MultiPointConstrUnionCard& card)
{
    m_id.set(row, card.id);
    m_sets.set(row, card.sets);
}

void MultiPointConstrUnionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_sets.keepRows(rows);
}

void MultiPointConstrUnionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> sets(m_sets.get(row).begin(), m_sets.get(row).end());
    bool changed = false;
    for (base::EntityId& set : sets)
    {
        const base::EntityId updated = visitor.visit("memberSetId", ReferenceTarget::ConstraintSet, set);
        changed = changed || updated != set;
        set = updated;
    }
    if (changed)
        m_sets.set(row, sets);
}

} // namespace oor::model
