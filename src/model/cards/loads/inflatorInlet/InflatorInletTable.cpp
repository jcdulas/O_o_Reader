#include "oor/model/cards/loads/inflatorInlet/InflatorInletTable.hpp"

namespace oor::model
{

std::optional<InflatorInletCard> InflatorInletFormat::read(FieldReader& reader)
{
    InflatorInletCard card;
    card.id = reader.requiredInteger(1, "id");
    card.inflatorId = reader.requiredInteger(2, "inflatorId");
    card.subsurfaceId = reader.requiredInteger(3, "subsurfaceId");
    card.inflatorKind =
        readRequiredKeyword<InflatorInletInflator>(reader, 4, "inflatorKind", INFLATOR_INLET_INFLATOR_WORDS);
    card.inflatorKindId = reader.requiredInteger(5, "inflatorKindId");
    card.coeff = readSourcedValue(reader, INFLATOR_INLET_COEFF_LAYOUT);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InflatorInletFormat::check(const InflatorInletCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.inflatorId, "inflatorId"))
        return problem;
    if (const auto problem = checkRequiredId(card.subsurfaceId, "subsurfaceId"))
        return problem;
    if (const auto problem = checkRequiredId(card.inflatorKindId, "inflatorKindId"))
        return problem;
    if (const auto problem = checkSourcedValue(card.coeff, INFLATOR_INLET_COEFF_LAYOUT, RealRule::NonNegative))
        return problem;
    if (card.coeff.source == ValueSource::Constant)
        return checkRealRange(card.coeff.constant, "areaCoefficient", 0.0, 1.0);
    return std::nullopt;
}

void InflatorInletFormat::write(const InflatorInletCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.inflatorId);
    writer.integer(3, card.subsurfaceId);
    writeKeyword(writer, 4, INFLATOR_INLET_INFLATOR_WORDS, card.inflatorKind);
    writer.integer(5, card.inflatorKindId);
    writeSourcedValue(writer, INFLATOR_INLET_COEFF_LAYOUT, card.coeff);
}

std::uint32_t InflatorInletTable::append(const InflatorInletCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_inflatorId.push(card.inflatorId);
    m_cards.push(card);
    return row;
}

void InflatorInletTable::store(std::uint32_t row, const InflatorInletCard& card)
{
    m_inflatorId.set(row, card.inflatorId);
    m_cards.set(row, card);
}

void InflatorInletTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_inflatorId.keepRows(rows);
    m_cards.keepRows(rows);
}

void InflatorInletTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_inflatorId.set(row, id);
    m_cards.edit(row).inflatorId = id;
}

void InflatorInletTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    InflatorInletCard& card = m_cards.edit(row);
    visitId(visitor, "subsurfaceId", ReferenceTarget::ContactSurface, card.subsurfaceId);
    visitId(visitor, "inflatorKindId", ReferenceTarget::Inflator, card.inflatorKindId);
    visitSourcedTable(visitor, "areaCoefficient", card.coeff);
}

} // namespace oor::model
