#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 12;

} // namespace

std::optional<AeroBody2Card> AeroBody2Format::read(FieldReader& reader)
{
    AeroBody2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.positionFrameId = reader.integer(3, "positionFrameId", 0);
    card.slenderElemCount = reader.integer(4, "slenderElemCount", 0);
    card.interferenceElemCount = reader.integer(5, "interferenceElemCount", 0);
    card.slenderListId = reader.integer(6, "slenderListId", 0);
    card.interferenceListId = reader.integer(7, "interferenceListId", 0);
    card.interferenceGroupId = reader.requiredInteger(8, "interferenceGroupId");
    card.point1 = {reader.real(9, "xCoord1", 0.0), reader.real(10, "yCoord1", 0.0), reader.real(11, "zCoord1", 0.0)};
    card.length = reader.requiredReal(12, "length");
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 12 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AeroBody2Format::check(const AeroBody2Card& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    if (const auto problem = checkOptionalId(card.positionFrameId, "positionFrameId"))
        return problem;
    if (card.slenderElemCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "slenderElemCount"};
    if (card.interferenceElemCount < 0)
        return CardProblem{"card.bad-value", "{field} cannot be negative", "interferenceElemCount"};
    if (const auto problem = checkOptionalId(card.slenderListId, "slenderListId"))
        return problem;
    if (const auto problem = checkOptionalId(card.interferenceListId, "interferenceListId"))
        return problem;
    // Both cuts of the body are needed: a number of equal divisions, or else an AEFACT list of division points.
    if (card.slenderElemCount == 0 && card.slenderListId == 0)
        return CardProblem{"card.missing-field", "{field} or {field:slenderListId} is required", "slenderElemCount"};
    if (card.interferenceElemCount == 0 && card.interferenceListId == 0)
        return CardProblem{"card.missing-field", "{field} or {field:interferenceListId} is required",
                           "interferenceElemCount"};
    if (card.interferenceGroupId < 0)
        return CardProblem{"card.bad-id", "{field} cannot be negative", "interferenceGroupId"};
    for (const double value : card.point1)
    {
        if (!std::isfinite(value))
            return CardProblem{"card.bad-value", "{field}, {field:yCoord1} and {field:zCoord1} must be finite numbers",
                               "xCoord1"};
    }
    return checkReal(card.length, "length", RealRule::Positive);
}

void AeroBody2Format::write(const AeroBody2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integerUnlessDefault(3, card.positionFrameId, 0);
    writer.integerUnlessDefault(4, card.slenderElemCount, 0);
    writer.integerUnlessDefault(5, card.interferenceElemCount, 0);
    writer.integerUnlessDefault(6, card.slenderListId, 0);
    writer.integerUnlessDefault(7, card.interferenceListId, 0);
    // IGID has no default and may be zero, so it is always written; so is the length of the body, which is on the
    // continuation line with the coordinates of the nose (zeros included).
    writer.integer(8, card.interferenceGroupId);
    writer.real(9, card.point1[0]);
    writer.real(10, card.point1[1]);
    writer.real(11, card.point1[2]);
    writer.real(12, card.length);
}

std::uint32_t AeroBody2Table::append(const AeroBody2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_positionFrameId.push(card.positionFrameId);
    m_slenderElemCount.push(card.slenderElemCount);
    m_interferenceElemCount.push(card.interferenceElemCount);
    m_slenderListId.push(card.slenderListId);
    m_interferenceListId.push(card.interferenceListId);
    m_interferenceGroupId.push(card.interferenceGroupId);
    m_point1.push(card.point1);
    m_length.push(card.length);
    return row;
}

AeroBody2Card AeroBody2Table::card(std::uint32_t row) const
{
    AeroBody2Card card;
    card.id = m_id.get(row);
    card.propId = m_propId.get(row);
    card.positionFrameId = m_positionFrameId.get(row);
    card.slenderElemCount = m_slenderElemCount.get(row);
    card.interferenceElemCount = m_interferenceElemCount.get(row);
    card.slenderListId = m_slenderListId.get(row);
    card.interferenceListId = m_interferenceListId.get(row);
    card.interferenceGroupId = m_interferenceGroupId.get(row);
    card.point1 = m_point1.get(row);
    card.length = m_length.get(row);
    return card;
}

void AeroBody2Table::store(std::uint32_t row, const AeroBody2Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_positionFrameId.set(row, card.positionFrameId);
    m_slenderElemCount.set(row, card.slenderElemCount);
    m_interferenceElemCount.set(row, card.interferenceElemCount);
    m_slenderListId.set(row, card.slenderListId);
    m_interferenceListId.set(row, card.interferenceListId);
    m_interferenceGroupId.set(row, card.interferenceGroupId);
    m_point1.set(row, card.point1);
    m_length.set(row, card.length);
}

void AeroBody2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_slenderElemCount.keepRows(rows);
    m_interferenceElemCount.keepRows(rows);
    m_slenderListId.keepRows(rows);
    m_interferenceListId.keepRows(rows);
    m_interferenceGroupId.keepRows(rows);
    m_point1.keepRows(rows);
    m_length.keepRows(rows);
}

void AeroBody2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId propId = m_propId.get(row); propId > 0)
        m_propId.set(row, visitor.visit("propId", ReferenceTarget::AeroProperty, propId));
    if (const base::EntityId positionFrameId = m_positionFrameId.get(row); positionFrameId > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, positionFrameId));
    if (const base::EntityId slenderListId = m_slenderListId.get(row); slenderListId > 0)
        m_slenderListId.set(row, visitor.visit("slenderListId", ReferenceTarget::AeroFactor, slenderListId));
    if (const base::EntityId interferenceListId = m_interferenceListId.get(row); interferenceListId > 0)
        m_interferenceListId.set(row,
                                 visitor.visit("interferenceListId", ReferenceTarget::AeroFactor, interferenceListId));
}

} // namespace oor::model
