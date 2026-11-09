#include "oor/model/cards/coordinate_systems/rectFrame2/RectFrame2Table.hpp"

namespace oor::model
{

std::optional<RectFrame2Card> RectFrame2Format::read(FieldReader& reader)
{
    RectFrame2Card card;
    card.id = reader.requiredInteger(1, "id");
    card.referenceFrameId = reader.integer(2, "referenceFrameId", 0);
    // The points have no default in the format reference; a blank coordinate is 0.0.
    card.pointA[0] = reader.real(3, "pointA1", 0.0);
    card.pointA[1] = reader.real(4, "pointA2", 0.0);
    card.pointA[2] = reader.real(5, "pointA3", 0.0);
    card.pointB[0] = reader.real(6, "pointB1", 0.0);
    card.pointB[1] = reader.real(7, "pointB2", 0.0);
    card.pointB[2] = reader.real(8, "pointB3", 0.0);
    card.pointC[0] = reader.real(9, "pointC1", 0.0);
    card.pointC[1] = reader.real(10, "pointC2", 0.0);
    card.pointC[2] = reader.real(11, "pointC3", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RectFrame2Format::check(const RectFrame2Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the coordinate system ID must be a positive integer", "id"};
    if (card.referenceFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "referenceFrameId"};
    if (card.referenceFrameId == card.id)
        return CardProblem{"card.bad-value", "a coordinate system cannot be defined with respect to itself",
                           "referenceFrameId"};
    return std::nullopt;
}

void RectFrame2Format::write(const RectFrame2Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.referenceFrameId, 0);
    // All nine coordinates are written, so that the continuation line the format reference requires is always there.
    for (int index = 0; index < 3; ++index)
    {
        writer.real(3 + index, card.pointA[static_cast<std::size_t>(index)]);
        writer.real(6 + index, card.pointB[static_cast<std::size_t>(index)]);
        writer.real(9 + index, card.pointC[static_cast<std::size_t>(index)]);
    }
}

std::uint32_t RectFrame2Table::append(const RectFrame2Card& card, base::CardKey key)
{
    ++m_version;
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_referenceFrameId.push(card.referenceFrameId);
    m_pointA.push(card.pointA);
    m_pointB.push(card.pointB);
    m_pointC.push(card.pointC);
    return row;
}

RectFrame2Card RectFrame2Table::card(std::uint32_t row) const
{
    return RectFrame2Card{m_id.get(row), m_referenceFrameId.get(row), m_pointA.get(row), m_pointB.get(row),
                          m_pointC.get(row)};
}

void RectFrame2Table::store(std::uint32_t row, const RectFrame2Card& card)
{
    ++m_version;
    m_id.set(row, card.id);
    m_referenceFrameId.set(row, card.referenceFrameId);
    m_pointA.set(row, card.pointA);
    m_pointB.set(row, card.pointB);
    m_pointC.set(row, card.pointC);
}

void RectFrame2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    ++m_version;
    m_id.keepRows(rows);
    m_referenceFrameId.keepRows(rows);
    m_pointA.keepRows(rows);
    m_pointB.keepRows(rows);
    m_pointC.keepRows(rows);
}

void RectFrame2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ++m_version;
    if (const base::EntityId referenceFrameId = m_referenceFrameId.get(row); referenceFrameId > 0)
        m_referenceFrameId.set(row, visitor.visit("referenceFrameId", ReferenceTarget::Coordinate, referenceFrameId));
}

} // namespace oor::model
