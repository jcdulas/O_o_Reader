#include "oor/model/cards/nodes/moduleMoveSequence/ModuleMoveSequenceTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

// The first move of the list is in field 2, after the MODID.
constexpr int FIRST_MOVE_FIELD = 2;

} // namespace

std::optional<ModuleMoveSequenceCard> ModuleMoveSequenceFormat::read(FieldReader& reader)
{
    ModuleMoveSequenceCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (int field = FIRST_MOVE_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        card.moveIds.push_back(reader.requiredInteger(field, "moveId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleMoveSequenceFormat::check(const ModuleMoveSequenceCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.moveIds.empty())
        return CardProblem{"card.missing-field", "the card needs at least one move ({field})", "moveId1"};
    for (const base::EntityId moveId : card.moveIds)
    {
        if (const auto problem = checkRequiredId(moveId, "moveId"))
            return problem;
    }
    return std::nullopt;
}

void ModuleMoveSequenceFormat::write(const ModuleMoveSequenceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_MOVE_FIELD;
    for (const base::EntityId moveId : card.moveIds)
        writer.integer(field++, moveId);
}

std::uint32_t ModuleMoveSequenceTable::append(const ModuleMoveSequenceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ModuleMoveSequenceTable::store(std::uint32_t row, const ModuleMoveSequenceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ModuleMoveSequenceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleMoveSequenceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ModuleMoveSequenceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The MODID is a module number, a plain integer: only the moves are references.
    for (base::EntityId& moveId : m_cards.edit(row).moveIds)
        visitId(visitor, "moveId", ReferenceTarget::ModuleMove, moveId);
}

} // namespace oor::model
