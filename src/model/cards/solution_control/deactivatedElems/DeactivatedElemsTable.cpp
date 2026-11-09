#include "oor/model/cards/solution_control/deactivatedElems/DeactivatedElemsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<DeactivatedElemsCard> DeactivatedElemsFormat::read(FieldReader& reader)
{
    DeactivatedElemsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemSetId = reader.requiredInteger(5, "elemSetId");
    // The cells the format draws blank: fields 2 to 4 and 6 to 8, and the end of the card.
    requireUnusedBlank(reader, {2, 3, 4, 6, 7, 8}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DeactivatedElemsFormat::check(const DeactivatedElemsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkRequiredId(card.elemSetId, "elemSetId");
}

void DeactivatedElemsFormat::write(const DeactivatedElemsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(5, card.elemSetId);
}

std::uint32_t DeactivatedElemsTable::append(const DeactivatedElemsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void DeactivatedElemsTable::store(std::uint32_t row, const DeactivatedElemsCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void DeactivatedElemsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void DeactivatedElemsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void DeactivatedElemsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "elemSetId", ReferenceTarget::Set, m_cards.edit(row).elemSetId);
}

} // namespace oor::model
