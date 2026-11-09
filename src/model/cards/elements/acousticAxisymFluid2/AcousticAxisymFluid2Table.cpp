#include "oor/model/cards/elements/acousticAxisymFluid2/AcousticAxisymFluid2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 7;
constexpr base::EntityId ID_LIMIT = 100000000; // an element ID is below it (format reference, EID)

} // namespace

std::optional<AcousticAxisymFluid2Card> AcousticAxisymFluid2Format::read(FieldReader& reader)
{
    AcousticAxisymFluid2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.fluidPoints = {reader.requiredInteger(2, "fluidPointId1"), reader.requiredInteger(3, "fluidPointId2")};
    // The grid of the format reference leaves fields 4 and 5 blank for this card (CAXIF3 and CAXIF4 use them).
    requireBlankFields(reader, {4, 5}, LAST_FIELD);
    card.density = reader.optionalReal(6, "density");
    card.bulkModulus = reader.optionalReal(7, "bulkModulus");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AcousticAxisymFluid2Format::check(const AcousticAxisymFluid2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (card.id >= ID_LIMIT)
        return CardProblem{"card.bad-id", "{field} must be an integer from 1 to 99999999", "id"};
    if (const auto problem = checkRequiredId(card.fluidPoints[0], "fluidPointId1"))
        return problem;
    if (const auto problem = checkRequiredId(card.fluidPoints[1], "fluidPointId2"))
        return problem;
    if (const auto problem = checkOptionalReal(card.density, "density", RealRule::Positive))
        return problem;
    return checkOptionalReal(card.bulkModulus, "bulkModulus", RealRule::NonNegative);
}

void AcousticAxisymFluid2Format::write(const AcousticAxisymFluid2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.fluidPoints[0]);
    writer.integer(3, card.fluidPoints[1]);
    if (card.density)
        writer.real(6, *card.density);
    if (card.bulkModulus)
        writer.real(7, *card.bulkModulus);
}

std::uint32_t AcousticAxisymFluid2Table::append(const AcousticAxisymFluid2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AcousticAxisymFluid2Table::store(std::uint32_t row, const AcousticAxisymFluid2Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AcousticAxisymFluid2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AcousticAxisymFluid2Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AcousticAxisymFluid2Table::visitReferences(std::uint32_t, ReferenceVisitor&)
{
    // The points are GRIDF fluid points, which no card of the library defines: there is nothing to follow.
}

} // namespace oor::model
