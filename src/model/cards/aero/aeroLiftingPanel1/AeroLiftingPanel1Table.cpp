#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"

#include "oor/model/cards/aero/AeroSupport.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 16;

} // namespace

std::optional<AeroLiftingPanel1Card> AeroLiftingPanel1Format::read(FieldReader& reader)
{
    AeroLiftingPanel1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.positionFrameId = reader.integer(3, "positionFrameId", 0);
    card.spanBoxCount = reader.integer(4, "spanBoxCount", 0);
    card.chordBoxCount = reader.integer(5, "chordBoxCount", 0);
    card.spanDivisionListId = reader.integer(6, "spanDivisionListId", 0);
    card.chordDivisionListId = reader.integer(7, "chordDivisionListId", 0);
    card.interferenceGroupId = reader.requiredInteger(8, "interferenceGroupId");
    card.point1 = {reader.real(9, "xCoord1", 0.0), reader.real(10, "yCoord1", 0.0), reader.real(11, "zCoord1", 0.0)};
    card.rootChord = reader.real(12, "rootChord", 0.0);
    card.point4 = {reader.real(13, "xCoord4", 0.0), reader.real(14, "yCoord4", 0.0), reader.real(15, "zCoord4", 0.0)};
    card.tipChord = reader.real(16, "tipChord", 0.0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 16 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroLiftingPanel1Format::check(const AeroLiftingPanel1Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.positionFrameId, "positionFrameId"))
        return problem;
    if (card.spanBoxCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "spanBoxCount"};
    if (card.chordBoxCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "chordBoxCount"};
    if (const auto problem = checkOptionalId(card.spanDivisionListId, "spanDivisionListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.chordDivisionListId, "chordDivisionListId"))
        return problem;
    // The boxes come from the number of divisions, or else from the AEFACT list (format reference fields 4 to 7).
    if (card.spanBoxCount == 0 && card.spanDivisionListId == 0)
        return CardProblem{"card.missing-field", "{field} or {field:spanDivisionListId} is required", "spanBoxCount"};
    if (card.chordBoxCount == 0 && card.chordDivisionListId == 0)
        return CardProblem{"card.missing-field", "{field} or {field:chordDivisionListId} is required", "chordBoxCount"};
    if (const auto problem = checkRequiredId(card.interferenceGroupId, "interferenceGroupId"))
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

void AeroLiftingPanel1Format::write(const AeroLiftingPanel1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.positionFrameId, 0);
    writer.integerUnlessDefault(4, card.spanBoxCount, 0);
    writer.integerUnlessDefault(5, card.chordBoxCount, 0);
    writer.integerUnlessDefault(6, card.spanDivisionListId, 0);
    writer.integerUnlessDefault(7, card.chordDivisionListId, 0);
    writer.integer(8, card.interferenceGroupId);
    // The continuation line is required, so the coordinates are always written, zeros included.
    writer.real(9, card.point1[0]);
    writer.real(10, card.point1[1]);
    writer.real(11, card.point1[2]);
    writer.real(12, card.rootChord);
    writer.real(13, card.point4[0]);
    writer.real(14, card.point4[1]);
    writer.real(15, card.point4[2]);
    writer.real(16, card.tipChord);
}

std::uint32_t AeroLiftingPanel1Table::append(const AeroLiftingPanel1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_positionFrameId.push(card.positionFrameId);
    m_spanBoxCount.push(card.spanBoxCount);
    m_chordBoxCount.push(card.chordBoxCount);
    m_spanDivisionListId.push(card.spanDivisionListId);
    m_chordDivisionListId.push(card.chordDivisionListId);
    m_interferenceGroupId.push(card.interferenceGroupId);
    m_point1.push(card.point1);
    m_rootChord.push(card.rootChord);
    m_point4.push(card.point4);
    m_tipChord.push(card.tipChord);
    return row;
}

AeroLiftingPanel1Card AeroLiftingPanel1Table::card(std::uint32_t row) const
{
    AeroLiftingPanel1Card card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.positionFrameId = m_positionFrameId.get(row);
    card.spanBoxCount = m_spanBoxCount.get(row);
    card.chordBoxCount = m_chordBoxCount.get(row);
    card.spanDivisionListId = m_spanDivisionListId.get(row);
    card.chordDivisionListId = m_chordDivisionListId.get(row);
    card.interferenceGroupId = m_interferenceGroupId.get(row);
    card.point1 = m_point1.get(row);
    card.rootChord = m_rootChord.get(row);
    card.point4 = m_point4.get(row);
    card.tipChord = m_tipChord.get(row);
    return card;
}

void AeroLiftingPanel1Table::store(std::uint32_t row, const AeroLiftingPanel1Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_positionFrameId.set(row, card.positionFrameId);
    m_spanBoxCount.set(row, card.spanBoxCount);
    m_chordBoxCount.set(row, card.chordBoxCount);
    m_spanDivisionListId.set(row, card.spanDivisionListId);
    m_chordDivisionListId.set(row, card.chordDivisionListId);
    m_interferenceGroupId.set(row, card.interferenceGroupId);
    m_point1.set(row, card.point1);
    m_rootChord.set(row, card.rootChord);
    m_point4.set(row, card.point4);
    m_tipChord.set(row, card.tipChord);
}

void AeroLiftingPanel1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_spanBoxCount.keepRows(rows);
    m_chordBoxCount.keepRows(rows);
    m_spanDivisionListId.keepRows(rows);
    m_chordDivisionListId.keepRows(rows);
    m_interferenceGroupId.keepRows(rows);
    m_point1.keepRows(rows);
    m_rootChord.keepRows(rows);
    m_point4.keepRows(rows);
    m_tipChord.keepRows(rows);
}

void AeroLiftingPanel1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::AeroProperty, propId));
    if (const base::EntityId positionFrameId = m_positionFrameId.get(row); positionFrameId > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, positionFrameId));
    if (const base::EntityId spanDivisionListId = m_spanDivisionListId.get(row); spanDivisionListId > 0)
        m_spanDivisionListId.set(row,
                                 visitor.visit("spanDivisionListId", ReferenceTarget::AeroFactor, spanDivisionListId));
    if (const base::EntityId chordDivisionListId = m_chordDivisionListId.get(row); chordDivisionListId > 0)
        m_chordDivisionListId.set(
            row, visitor.visit("chordDivisionListId", ReferenceTarget::AeroFactor, chordDivisionListId));
}

} // namespace oor::model
