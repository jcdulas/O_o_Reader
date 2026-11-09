#include "oor/model/cards/dynamics/initialValuesUserRoutine/InitialValuesUserRoutineTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<InitialValuesUserRoutineCard> InitialValuesUserRoutineFormat::read(FieldReader& reader)
{
    InitialValuesUserRoutineCard card;
    card.id = reader.requiredInteger(1, "id");
    card.group = std::string(reader.text(2));
    card.routineName =
        readKeyword(reader, 3, "routineName", INITIAL_VALUES_ROUTINE_WORDS, InitialValuesUserRoutineRoutine::Exinit);
    card.elementSetId = reader.requiredInteger(4, "elementSetId");
    card.target =
        readKeyword(reader, 5, "target", INITIAL_VALUES_TARGET_WORDS, InitialValuesUserRoutineTarget::Element);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "the card has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialValuesUserRoutineFormat::check(const InitialValuesUserRoutineCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkNameWord(card.group, "group"))
        return problem;
    return checkRequiredId(card.elementSetId, "elementSetId");
}

void InitialValuesUserRoutineFormat::write(const InitialValuesUserRoutineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.group);
    writeKeywordUnlessDefault(writer, 3, INITIAL_VALUES_ROUTINE_WORDS, card.routineName,
                              InitialValuesUserRoutineRoutine::Exinit);
    writer.integer(4, card.elementSetId);
    writeKeywordUnlessDefault(writer, 5, INITIAL_VALUES_TARGET_WORDS, card.target,
                              InitialValuesUserRoutineTarget::Element);
}

std::uint32_t InitialValuesUserRoutineTable::append(const InitialValuesUserRoutineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void InitialValuesUserRoutineTable::store(std::uint32_t row, const InitialValuesUserRoutineCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void InitialValuesUserRoutineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void InitialValuesUserRoutineTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void InitialValuesUserRoutineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "elementSetId", ReferenceTarget::Set, m_cards.edit(row).elementSetId);
}

} // namespace oor::model
