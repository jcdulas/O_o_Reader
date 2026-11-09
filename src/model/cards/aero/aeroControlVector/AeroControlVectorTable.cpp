#include "oor/model/cards/aero/aeroControlVector/AeroControlVectorTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_VARIABLE_FIELD = 9;

} // namespace

std::optional<AeroControlVectorCard> AeroControlVectorFormat::read(FieldReader& reader)
{
    AeroControlVectorCard card;
    card.id = reader.requiredInteger(1, "id");
    // Fields 2 to 8 are the blank cells of the first line; the pairs are on the continuation lines, to the end of the
    // card.
    requireBlankFields(reader, {2, 3, 4, 5, 6, 7, 8}, reader.slotCount());
    card.variables = readLabeledValues(reader, FIRST_VARIABLE_FIELD, "labels", "values");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroControlVectorFormat::check(const AeroControlVectorCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkLabeledValues(card.variables, "labels", "values");
}

void AeroControlVectorFormat::write(const AeroControlVectorCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeLabeledValues(writer, FIRST_VARIABLE_FIELD, card.variables);
}

std::uint32_t AeroControlVectorTable::append(const AeroControlVectorCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroControlVectorTable::store(std::uint32_t row, const AeroControlVectorCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroControlVectorTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroControlVectorTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
