#include "oor/model/cards/nodes/acousticSlotControl/AcousticSlotControlTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5; // MD

} // namespace

std::optional<AcousticSlotControlCard> AcousticSlotControlFormat::read(FieldReader& reader)
{
    AcousticSlotControlCard card;
    card.defaultDensity = reader.optionalReal(1, "defaultDensity");
    card.defaultBulkModulus = reader.optionalReal(2, "defaultBulkModulus");
    card.harmonic = reader.optionalInteger(3, "harmonic");
    card.defaultSlotWidth = reader.optionalReal(4, "defaultSlotWidth");
    card.defaultSlotCount = reader.optionalInteger(5, "defaultSlotCount");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 5 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticSlotControlFormat::check(const AcousticSlotControlCard& card)
{
    if (const auto problem = checkOptionalReal(card.defaultDensity, "defaultDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.defaultBulkModulus, "defaultBulkModulus", RealRule::NonNegative))
        return problem;
    if (card.harmonic && *card.harmonic < 0)
        return CardProblem{"card.bad-value", "{field} must not be negative", "harmonic"};
    if (const auto problem = checkOptionalReal(card.defaultSlotWidth, "defaultSlotWidth", RealRule::NonNegative))
        return problem;
    if (card.defaultSlotCount && *card.defaultSlotCount < 0)
        return CardProblem{"card.bad-value", "{field} must not be negative", "defaultSlotCount"};
    return std::nullopt;
}

void AcousticSlotControlFormat::write(const AcousticSlotControlCard& card, FieldWriter& writer)
{
    if (card.defaultDensity)
        writer.real(1, *card.defaultDensity);
    if (card.defaultBulkModulus)
        writer.real(2, *card.defaultBulkModulus);
    if (card.harmonic)
        writer.integer(3, *card.harmonic);
    if (card.defaultSlotWidth)
        writer.real(4, *card.defaultSlotWidth);
    if (card.defaultSlotCount)
        writer.integer(5, *card.defaultSlotCount);
}

std::uint32_t AcousticSlotControlTable::append(const AcousticSlotControlCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_defaultDensity.push(realToColumn(card.defaultDensity));
    m_defaultBulkModulus.push(realToColumn(card.defaultBulkModulus));
    m_harmonic.push(card.harmonic.value_or(NO_VALUE));
    m_defaultSlotWidth.push(realToColumn(card.defaultSlotWidth));
    m_defaultSlotCount.push(card.defaultSlotCount.value_or(NO_VALUE));
    return row;
}

AcousticSlotControlCard AcousticSlotControlTable::card(std::uint32_t row) const
{
    AcousticSlotControlCard card;
    card.defaultDensity = realFromColumn(m_defaultDensity.get(row));
    card.defaultBulkModulus = realFromColumn(m_defaultBulkModulus.get(row));
    if (const std::int32_t n = m_harmonic.get(row); n != NO_VALUE)
        card.harmonic = n;
    card.defaultSlotWidth = realFromColumn(m_defaultSlotWidth.get(row));
    if (const std::int32_t slotCount = m_defaultSlotCount.get(row); slotCount != NO_VALUE)
        card.defaultSlotCount = slotCount;
    return card;
}

void AcousticSlotControlTable::store(std::uint32_t row, const AcousticSlotControlCard& card)
{
    m_defaultDensity.set(row, realToColumn(card.defaultDensity));
    m_defaultBulkModulus.set(row, realToColumn(card.defaultBulkModulus));
    m_harmonic.set(row, card.harmonic.value_or(NO_VALUE));
    m_defaultSlotWidth.set(row, realToColumn(card.defaultSlotWidth));
    m_defaultSlotCount.set(row, card.defaultSlotCount.value_or(NO_VALUE));
}

void AcousticSlotControlTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_defaultDensity.keepRows(rows);
    m_defaultBulkModulus.keepRows(rows);
    m_harmonic.keepRows(rows);
    m_defaultSlotWidth.keepRows(rows);
    m_defaultSlotCount.keepRows(rows);
}

void AcousticSlotControlTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-acoustic-slot-control",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
