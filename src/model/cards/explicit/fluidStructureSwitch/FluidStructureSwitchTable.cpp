#include "oor/model/cards/explicit/fluidStructureSwitch/FluidStructureSwitchTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<FluidStructureSwitchCard> FluidStructureSwitchFormat::read(FieldReader& reader)
{
    FluidStructureSwitchCard card;
    card.id = reader.requiredInteger(1, "id");
    card.tableId = reader.integer(2, "tableId", 0);
    card.eulerInitialization =
        readKeyword(reader, 3, "eulerInitialization", EULER_INITIALIZATION_WORDS, EulerInitialization::AtCycleZero);
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 3 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidStructureSwitchFormat::check(const FluidStructureSwitchCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkOptionalId(card.tableId, "tableId");
}

void FluidStructureSwitchFormat::write(const FluidStructureSwitchCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.tableId, 0);
    writeKeywordUnlessDefault(writer, 3, EULER_INITIALIZATION_WORDS, card.eulerInitialization,
                              EulerInitialization::AtCycleZero);
}

std::uint32_t FluidStructureSwitchTable::append(const FluidStructureSwitchCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void FluidStructureSwitchTable::store(std::uint32_t row, const FluidStructureSwitchCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void FluidStructureSwitchTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void FluidStructureSwitchTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FluidStructureSwitchTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "tableId", ReferenceTarget::DynamicTable, m_cards.edit(row).tableId);
}

} // namespace oor::model
