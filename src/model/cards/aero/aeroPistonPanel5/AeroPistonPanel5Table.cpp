#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;
constexpr std::int32_t LAST_THEORY = 2;

} // namespace

std::optional<AeroPistonPanel5Card> AeroPistonPanel5Format::read(FieldReader& reader)
{
    AeroPistonPanel5Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.positionFrameId = reader.integer(3, "positionFrameId", 0);
    card.spanBoxCount = reader.integer(4, "spanBoxCount", 0);
    card.spanDivisionListId = reader.integer(5, "spanDivisionListId", 0);
    card.theory = reader.integer(6, "theory", 0);
    card.thicknessIntegralListId = reader.integer(7, "thicknessIntegralListId", 0);
    card.point1 = {reader.real(9, "xCoord1", 0.0), reader.real(10, "yCoord1", 0.0), reader.real(11, "zCoord1", 0.0)};
    card.rootChord = reader.real(12, "rootChord", 0.0);
    card.point4 = {reader.real(13, "xCoord4", 0.0), reader.real(14, "yCoord4", 0.0), reader.real(15, "zCoord4", 0.0)};
    card.tipChord = reader.real(16, "tipChord", 0.0);
    // Field 8 is a blank cell of the layout, and the card ends with X43.
    requireBlankFields(reader, {8}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroPistonPanel5Format::check(const AeroPistonPanel5Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.positionFrameId, "positionFrameId"))
        return problem;
    if (card.spanBoxCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "spanBoxCount"};
    if (const auto problem = checkOptionalId(card.spanDivisionListId, "spanDivisionListId"))
        return problem;
    // The strips come from the number of divisions, or else from the AEFACT list (format reference fields 4 and 5).
    if (card.spanBoxCount == 0 && card.spanDivisionListId == 0)
        return CardProblem{"card.missing-field", "{field} or {field:spanDivisionListId} is required", "spanBoxCount"};
    if (card.theory < 0 || card.theory > LAST_THEORY)
        return CardProblem{"card.bad-value", "{field} must be 0, 1 or 2", "theory"};
    if (const auto problem = checkOptionalId(card.thicknessIntegralListId, "thicknessIntegralListId"))
        return problem;
    for (const double value : card.point1)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "{field}, {field:yCoord1} and {field:zCoord1} must be finite numbers",
                               "xCoord1"};
    }
    for (const double value : card.point4)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "{field}, {field:yCoord4} and {field:zCoord4} must be finite numbers",
                               "xCoord4"};
    }
    // The chords are lengths: not negative, and not both zero (a panel with no area).
    if (const auto problem = checkReal(card.rootChord, "rootChord", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.tipChord, "tipChord", RealRule::NonNegative))
        return problem;
    if (card.rootChord == 0.0 && card.tipChord == 0.0)
        return CardProblem{"card.bad-value", "{field} and {field:tipChord} cannot both be zero", "rootChord"};
    return std::nullopt;
}

void AeroPistonPanel5Format::write(const AeroPistonPanel5Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.positionFrameId, 0);
    writer.integerUnlessDefault(4, card.spanBoxCount, 0);
    writer.integerUnlessDefault(5, card.spanDivisionListId, 0);
    writer.integerUnlessDefault(6, card.theory, 0);
    writer.integerUnlessDefault(7, card.thicknessIntegralListId, 0);
    // The continuation line holds the geometry, so its fields are always written, zeros included.
    writer.real(9, card.point1[0]);
    writer.real(10, card.point1[1]);
    writer.real(11, card.point1[2]);
    writer.real(12, card.rootChord);
    writer.real(13, card.point4[0]);
    writer.real(14, card.point4[1]);
    writer.real(15, card.point4[2]);
    writer.real(16, card.tipChord);
}

std::uint32_t AeroPistonPanel5Table::append(const AeroPistonPanel5Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void AeroPistonPanel5Table::store(std::uint32_t row, const AeroPistonPanel5Card& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void AeroPistonPanel5Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void AeroPistonPanel5Table::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void AeroPistonPanel5Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    AeroPistonPanel5Card& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::AeroProperty, card.propId);
    visitId(visitor, "positionFrameId", ReferenceTarget::Coordinate, card.positionFrameId);
    visitId(visitor, "spanDivisionListId", ReferenceTarget::AeroFactor, card.spanDivisionListId);
    visitId(visitor, "thicknessIntegralListId", ReferenceTarget::AeroFactor, card.thicknessIntegralListId);
}

} // namespace oor::model
