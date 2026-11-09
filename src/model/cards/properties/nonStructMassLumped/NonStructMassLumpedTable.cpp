#include "oor/model/cards/properties/nonStructMassLumped/NonStructMassLumpedTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 3;

} // namespace

std::optional<NonStructMassLumpedCard> NonStructMassLumpedFormat::read(FieldReader& reader)
{
    NonStructMassLumpedCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readNonStructMassType(reader, 2);
    card.pairs = readNonStructMassPairs(reader, FIRST_PAIR_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonStructMassLumpedFormat::check(const NonStructMassLumpedCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the non-structural mass set ID must be a positive integer", "id"};
    if (auto problem = checkNonStructMassType(card.type))
        return problem;
    return checkNonStructMassPairs(card.pairs);
}

void NonStructMassLumpedFormat::write(const NonStructMassLumpedCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.type);
    writeNonStructMassPairs(writer, FIRST_PAIR_FIELD, card.pairs);
}

std::uint32_t NonStructMassLumpedTable::append(const NonStructMassLumpedCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_type.push(card.type);
    std::vector<base::EntityId> ids;
    std::vector<double> values;
    for (const NonStructMassPair& pair : card.pairs)
    {
        ids.push_back(pair.id);
        values.push_back(pair.value);
    }
    m_ids.push(ids);
    m_values.push(values);
    return row;
}

NonStructMassLumpedCard NonStructMassLumpedTable::card(std::uint32_t row) const
{
    NonStructMassLumpedCard card;
    card.id = m_id.get(row);
    card.type = m_type.get(row);
    const auto ids = m_ids.get(row);
    const auto values = m_values.get(row);
    for (std::size_t index = 0; index < ids.size(); ++index)
        card.pairs.push_back(NonStructMassPair{ids[index], values[index]});
    return card;
}

void NonStructMassLumpedTable::store(std::uint32_t row, const NonStructMassLumpedCard& card)
{
    m_id.set(row, card.id);
    m_type.set(row, card.type);
    std::vector<base::EntityId> ids;
    std::vector<double> values;
    for (const NonStructMassPair& pair : card.pairs)
    {
        ids.push_back(pair.id);
        values.push_back(pair.value);
    }
    m_ids.set(row, ids);
    m_values.set(row, values);
}

void NonStructMassLumpedTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_type.keepRows(rows);
    m_ids.keepRows(rows);
    m_values.keepRows(rows);
}

void NonStructMassLumpedTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitNonStructMassIds(m_ids, row, visitor, m_type.get(row));
}

} // namespace oor::model
