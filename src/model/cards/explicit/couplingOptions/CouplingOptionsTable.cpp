#include "oor/model/cards/explicit/couplingOptions/CouplingOptionsTable.hpp"

namespace oor::model
{

std::optional<CouplingOptionsCard> CouplingOptionsFormat::read(FieldReader& reader)
{
    CouplingOptionsCard card;
    card.id = reader.requiredInteger(1, "id");
    card.setId = reader.requiredInteger(2, "setId");
    card.subsurfaceId = reader.integer(3, "subsurfaceId", 0);
    card.factor = readSourcedValue(reader, COUPLING_OPTIONS_FACTOR_LAYOUT);
    card.coverPressure = readSourcedValue(reader, COUPLING_OPTIONS_COVER_PRESSURE_LAYOUT);
    // format reference COUOPT: PLCOVER and PLCOVERV start the continuation line; the rest of the first line is blank.
    for (const int field : {6, 7, 8})
    {
        if (!reader.isBlank(field) && !reader.failed())
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of a " + std::string(reader.cardName()) +
                             " is blank: " + std::string("coverPressureSource") + " and " +
                             std::string("coverPressure") + " are on the continuation line (fields 9, 10)",
                         "coverPressureSource");
    }
    if (reader.lastUsedField() > COUPLING_OPTIONS_COVER_PRESSURE_LAYOUT.valueField)
        reader.error(
            "card.bad-value",
            "a " + std::string(reader.cardName()) + " ends with " + std::string("coverPressure") + ", its field 10",
            "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CouplingOptionsFormat::check(const CouplingOptionsCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.setId, "setId"))
        return problem;
    if (const auto problem = checkOptionalId(card.subsurfaceId, "subsurfaceId"))
        return problem;
    if (const auto problem = checkSourcedValue(card.factor, COUPLING_OPTIONS_FACTOR_LAYOUT, RealRule::NonNegative))
        return problem;
    return checkSourcedValue(card.coverPressure, COUPLING_OPTIONS_COVER_PRESSURE_LAYOUT, RealRule::NonNegative);
}

void CouplingOptionsFormat::write(const CouplingOptionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.setId);
    writer.integerUnlessDefault(3, card.subsurfaceId, 0);
    writeSourcedValue(writer, COUPLING_OPTIONS_FACTOR_LAYOUT, card.factor);
    writeSourcedValue(writer, COUPLING_OPTIONS_COVER_PRESSURE_LAYOUT, card.coverPressure);
}

std::uint32_t CouplingOptionsTable::append(const CouplingOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_setId.push(card.setId);
    m_cards.push(card);
    return row;
}

void CouplingOptionsTable::store(std::uint32_t row, const CouplingOptionsCard& card)
{
    m_setId.set(row, card.setId);
    m_cards.set(row, card);
}

void CouplingOptionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_setId.keepRows(rows);
    m_cards.keepRows(rows);
}

void CouplingOptionsTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_setId.set(row, id);
    m_cards.edit(row).setId = id;
}

void CouplingOptionsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    CouplingOptionsCard& card = m_cards.edit(row);
    visitId(visitor, "subsurfaceId", ReferenceTarget::ContactSurface, card.subsurfaceId);
    visitSourcedTable(visitor, "coverPressure", card.coverPressure);
}

} // namespace oor::model
