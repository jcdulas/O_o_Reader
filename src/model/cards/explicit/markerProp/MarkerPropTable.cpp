#include "oor/model/cards/explicit/markerProp/MarkerPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MarkerPropCard> MarkerPropFormat::read(FieldReader& reader)
{
    MarkerPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "markerKind", MARKER_PROP_TYPE_WORDS, MarkerPropType::Fixed);
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MarkerPropFormat::check(const MarkerPropCard& card)
{
    return checkRequiredId(card.id, "id");
}

void MarkerPropFormat::write(const MarkerPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, MARKER_PROP_TYPE_WORDS, card.type, MarkerPropType::Fixed);
}

std::uint32_t MarkerPropTable::append(const MarkerPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MarkerPropTable::store(std::uint32_t row, const MarkerPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MarkerPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MarkerPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
