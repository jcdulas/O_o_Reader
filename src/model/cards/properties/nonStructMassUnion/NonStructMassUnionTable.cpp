#include "oor/model/cards/properties/nonStructMassUnion/NonStructMassUnionTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_SET_FIELD = 2;

} // namespace

std::optional<NonStructMassUnionCard> NonStructMassUnionFormat::read(FieldReader& reader)
{
    NonStructMassUnionCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (int field = FIRST_SET_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        card.sets.push_back(reader.requiredInteger(field, "memberSetId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonStructMassUnionFormat::check(const NonStructMassUnionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the non-structural mass set ID must be a positive integer", "id"};
    if (card.sets.empty())
        return CardProblem{"card.missing-field", "the card needs at least one set", "memberSetId1"};
    for (const base::EntityId set : card.sets)
    {
        if (set <= 0)
            return CardProblem{"card.bad-id", "a set ID must be a positive integer", "memberSetId1"};
        if (set == card.id)
            return CardProblem{"card.bad-value",
                               "the card cannot refer to its own set ID (" + CardProblem::fieldNamed("id") + ")",
                               "memberSetId1"};
    }
    return std::nullopt;
}

void NonStructMassUnionFormat::write(const NonStructMassUnionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_SET_FIELD;
    for (const base::EntityId set : card.sets)
        writer.integer(field++, set);
}

std::uint32_t NonStructMassUnionTable::append(const NonStructMassUnionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_sets.push(card.sets);
    return row;
}

NonStructMassUnionCard NonStructMassUnionTable::card(std::uint32_t row) const
{
    NonStructMassUnionCard card;
    card.id = m_id.get(row);
    const auto sets = m_sets.get(row);
    card.sets.assign(sets.begin(), sets.end());
    return card;
}

void NonStructMassUnionTable::store(std::uint32_t row, const NonStructMassUnionCard& card)
{
    m_id.set(row, card.id);
    m_sets.set(row, card.sets);
}

void NonStructMassUnionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_sets.keepRows(rows);
}

} // namespace oor::model
