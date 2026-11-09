#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/superelements/SuperelementSupport.hpp"

namespace oor::model
{

std::optional<SuperelementBulkOptionsCard> SuperelementBulkOptionsFormat::read(FieldReader& reader)
{
    SuperelementBulkOptionsCard card;
    card.superelementId = reader.requiredInteger(1, "superelementId");
    card.partType =
        readRequiredKeyword<SuperelementBulkOptionsType>(reader, 2, "partType", SUPERELEMENT_BULK_TYPE_WORDS);
    card.referenceSuperelementId = reader.integer(3, "referenceSuperelementId", 0);
    card.method = readKeyword<SuperelementBulkOptionsMethod>(reader, 4, "method", SUPERELEMENT_BULK_METHOD_WORDS,
                                                             SuperelementBulkOptionsMethod::Auto);
    card.tolerance = reader.real(5, "tolerance", 0.0);
    card.location = readKeyword<SuperelementBulkOptionsLocation>(
        reader, 6, "location", SUPERELEMENT_BULK_LOCATION_WORDS, SuperelementBulkOptionsLocation::Yes);
    card.unitNumber = reader.integer(7, "unitNumber", 0);
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has 7 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SuperelementBulkOptionsFormat::check(const SuperelementBulkOptionsCard& card)
{
    if (const auto problem = checkSuperelementId(card.superelementId, "superelementId", true))
        return problem;
    if (const auto problem = checkOptionalId(card.referenceSuperelementId, "referenceSuperelementId"))
        return problem;
    if (const auto problem = checkReal(card.tolerance, "tolerance", RealRule::NonNegative))
        return problem;
    return checkOptionalId(card.unitNumber, "unitNumber");
}

void SuperelementBulkOptionsFormat::write(const SuperelementBulkOptionsCard& card, FieldWriter& writer)
{
    writer.integer(1, card.superelementId);
    writeKeyword(writer, 2, SUPERELEMENT_BULK_TYPE_WORDS, card.partType);
    writer.integerUnlessDefault(3, card.referenceSuperelementId, 0);
    writeKeywordUnlessDefault(writer, 4, SUPERELEMENT_BULK_METHOD_WORDS, card.method,
                              SuperelementBulkOptionsMethod::Auto);
    writer.realUnlessDefault(5, card.tolerance, 0.0);
    writeKeywordUnlessDefault(writer, 6, SUPERELEMENT_BULK_LOCATION_WORDS, card.location,
                              SuperelementBulkOptionsLocation::Yes);
    writer.integerUnlessDefault(7, card.unitNumber, 0);
}

std::uint32_t SuperelementBulkOptionsTable::append(const SuperelementBulkOptionsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_superelementId.push(card.superelementId);
    m_partType.push(static_cast<std::uint8_t>(card.partType));
    m_referenceSuperelementId.push(card.referenceSuperelementId);
    m_method.push(static_cast<std::uint8_t>(card.method));
    m_tolerance.push(card.tolerance);
    m_location.push(static_cast<std::uint8_t>(card.location));
    m_unitNumber.push(card.unitNumber);
    return row;
}

SuperelementBulkOptionsCard SuperelementBulkOptionsTable::card(std::uint32_t row) const
{
    SuperelementBulkOptionsCard card;
    card.superelementId = m_superelementId.get(row);
    card.partType = static_cast<SuperelementBulkOptionsType>(m_partType.get(row));
    card.referenceSuperelementId = m_referenceSuperelementId.get(row);
    card.method = static_cast<SuperelementBulkOptionsMethod>(m_method.get(row));
    card.tolerance = m_tolerance.get(row);
    card.location = static_cast<SuperelementBulkOptionsLocation>(m_location.get(row));
    card.unitNumber = m_unitNumber.get(row);
    return card;
}

void SuperelementBulkOptionsTable::store(std::uint32_t row, const SuperelementBulkOptionsCard& card)
{
    m_superelementId.set(row, card.superelementId);
    m_partType.set(row, static_cast<std::uint8_t>(card.partType));
    m_referenceSuperelementId.set(row, card.referenceSuperelementId);
    m_method.set(row, static_cast<std::uint8_t>(card.method));
    m_tolerance.set(row, card.tolerance);
    m_location.set(row, static_cast<std::uint8_t>(card.location));
    m_unitNumber.set(row, card.unitNumber);
}

void SuperelementBulkOptionsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_superelementId.keepRows(rows);
    m_partType.keepRows(rows);
    m_referenceSuperelementId.keepRows(rows);
    m_method.keepRows(rows);
    m_tolerance.keepRows(rows);
    m_location.keepRows(rows);
    m_unitNumber.keepRows(rows);
}

void SuperelementBulkOptionsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    base::EntityId referenceSuperelementId = m_referenceSuperelementId.get(row);
    visitSuperelement(visitor, "referenceSuperelementId", referenceSuperelementId);
    m_referenceSuperelementId.set(row, referenceSuperelementId);
}

} // namespace oor::model
