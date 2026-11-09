#include "oor/model/cards/loads/wettedSurfaceLoad/WettedSurfaceLoadTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"
#include "oor/model/cards/solution_control/ExternalName.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 3;

} // namespace

std::optional<WettedSurfaceLoadCard> WettedSurfaceLoadFormat::read(FieldReader& reader)
{
    WettedSurfaceLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.wettedSurfaceId = reader.requiredInteger(2, "wettedSurfaceId");
    card.serviceName = std::string(reader.text(3));
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> WettedSurfaceLoadFormat::check(const WettedSurfaceLoadCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.wettedSurfaceId, "wettedSurfaceId"))
        return problem;
    return checkExternalName(card.serviceName, "serviceName");
}

void WettedSurfaceLoadFormat::write(const WettedSurfaceLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.wettedSurfaceId);
    writer.text(3, card.serviceName);
}

std::uint32_t WettedSurfaceLoadTable::append(const WettedSurfaceLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void WettedSurfaceLoadTable::store(std::uint32_t row, const WettedSurfaceLoadCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void WettedSurfaceLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void WettedSurfaceLoadTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void WettedSurfaceLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The WLID is the TLOAD1's EXCITEID that names this card, so it is not a reference of the card.
    visitId(visitor, "wettedSurfaceId", ReferenceTarget::WettedSurface, m_cards.edit(row).wettedSurfaceId);
}

} // namespace oor::model
