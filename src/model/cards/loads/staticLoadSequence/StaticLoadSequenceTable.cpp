#include "oor/model/cards/loads/staticLoadSequence/StaticLoadSequenceTable.hpp"

namespace oor::model
{

std::optional<StaticLoadSequenceCard> StaticLoadSequenceFormat::read(FieldReader& reader)
{
    StaticLoadSequenceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.excitationId = reader.requiredInteger(2, "excitationId");
    card.loadId = reader.integer(3, "loadId", 0);
    card.tableId = reader.integer(4, "tableId", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> StaticLoadSequenceFormat::check(const StaticLoadSequenceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.excitationId <= 0)
        return CardProblem{"card.bad-id", "the excitation set ID must be a positive integer", "excitationId"};
    if (card.loadId < 0)
        return CardProblem{"card.bad-id", "the load set ID cannot be negative", "loadId"};
    if (card.tableId < 0)
        return CardProblem{"card.bad-id", "the temperature set ID cannot be negative", "tableId"};
    // LID and TID may both be blank in a superelement analysis, so a card with neither is accepted.
    return std::nullopt;
}

void StaticLoadSequenceFormat::write(const StaticLoadSequenceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.excitationId);
    writer.integerUnlessDefault(3, card.loadId, 0);
    writer.integerUnlessDefault(4, card.tableId, 0);
}

std::uint32_t StaticLoadSequenceTable::append(const StaticLoadSequenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_excitationId.push(card.excitationId);
    m_loadId.push(card.loadId);
    m_tableId.push(card.tableId);
    return row;
}

StaticLoadSequenceCard StaticLoadSequenceTable::card(std::uint32_t row) const
{
    return StaticLoadSequenceCard{m_id.get(row), m_excitationId.get(row), m_loadId.get(row), m_tableId.get(row)};
}

void StaticLoadSequenceTable::store(std::uint32_t row, const StaticLoadSequenceCard& card)
{
    m_id.set(row, card.id);
    m_excitationId.set(row, card.excitationId);
    m_loadId.set(row, card.loadId);
    m_tableId.set(row, card.tableId);
}

void StaticLoadSequenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_excitationId.keepRows(rows);
    m_loadId.keepRows(rows);
    m_tableId.keepRows(rows);
}

void StaticLoadSequenceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_excitationId.set(row, visitor.visit("excitationId", ReferenceTarget::ExcitationDefined, m_excitationId.get(row)));
    if (const base::EntityId loadId = m_loadId.get(row); loadId > 0)
        m_loadId.set(row, visitor.visit("loadId", ReferenceTarget::LoadCombinationSet, loadId));
    // A temperature set is a set of the Load category (TEMP, TEMPD, ...): Case Control selects it with TEMPERATURE.
    if (const base::EntityId tableId = m_tableId.get(row); tableId > 0)
        m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::LoadCombinationSet, tableId));
}

} // namespace oor::model
