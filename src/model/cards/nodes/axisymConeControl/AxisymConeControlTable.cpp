#include "oor/model/cards/nodes/axisymConeControl/AxisymConeControlTable.hpp"

#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 1; // H

} // namespace

std::optional<AxisymConeControlCard> AxisymConeControlFormat::read(FieldReader& reader)
{
    AxisymConeControlCard card;
    card.highestHarmonic = reader.requiredInteger(1, "highestHarmonic");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 1 field", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymConeControlFormat::check(const AxisymConeControlCard& card)
{
    if (card.highestHarmonic < 0 || card.highestHarmonic > AxisymConeControlCard::MAX_HARMONIC)
        return CardProblem{"card.bad-value", "{field} must be an integer from 0 to 997", "highestHarmonic"};
    return std::nullopt;
}

void AxisymConeControlFormat::write(const AxisymConeControlCard& card, FieldWriter& writer)
{
    writer.integer(1, card.highestHarmonic);
}

std::uint32_t AxisymConeControlTable::append(const AxisymConeControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_highestHarmonic.push(card.highestHarmonic);
    return row;
}

AxisymConeControlCard AxisymConeControlTable::card(std::uint32_t row) const
{
    return AxisymConeControlCard{m_highestHarmonic.get(row)};
}

void AxisymConeControlTable::store(std::uint32_t row, const AxisymConeControlCard& card)
{
    m_highestHarmonic.set(row, card.highestHarmonic);
}

void AxisymConeControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_highestHarmonic.keepRows(rows);
}

void AxisymConeControlTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-axisym-cone-control",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
