#include "oor/model/cards/nodes/geomPoint/GeomPointTable.hpp"

namespace oor::model
{

std::optional<GeomPointCard> GeomPointFormat::read(FieldReader& reader)
{
    GeomPointCard card;
    card.id = reader.requiredInteger(1, "id");
    card.positionFrameId = reader.integer(2, "positionFrameId", 0);
    card.xyz.x = reader.real(3, "position1", 0.0);
    card.xyz.y = reader.real(4, "position2", 0.0);
    card.xyz.z = reader.real(5, "position3", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeomPointFormat::check(const GeomPointCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.positionFrameId < 0)
        return CardProblem{"card.bad-value", "{field} must be 0 or a coordinate system ID", "positionFrameId"};
    return std::nullopt;
}

void GeomPointFormat::write(const GeomPointCard& card, FieldWriter& writer)
{
    // the POINTs of an arbitrary beam cross-section (a SET1 or SET3 of a PBRSECT or
    // PBMSECT) leave CP and X3 blank. A blank CP is the basic system and a blank X3 is 0.0, so both are written blank
    // when they hold their default, whatever the option of omitting defaults says.
    writer.integer(1, card.id);
    if (card.positionFrameId != 0)
        writer.integer(2, card.positionFrameId);
    writer.real(3, card.xyz.x);
    writer.real(4, card.xyz.y);
    if (card.xyz.z != 0.0)
        writer.real(5, card.xyz.z);
}

std::uint32_t GeomPointTable::append(const GeomPointCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_positionFrameId.push(card.positionFrameId);
    m_xyz.push({card.xyz.x, card.xyz.y, card.xyz.z});
    return row;
}

GeomPointCard GeomPointTable::card(std::uint32_t row) const
{
    const std::array<double, 3> xyz = m_xyz.get(row);
    return GeomPointCard{m_id.get(row), m_positionFrameId.get(row), base::Vec3{xyz[0], xyz[1], xyz[2]}};
}

void GeomPointTable::store(std::uint32_t row, const GeomPointCard& card)
{
    m_id.set(row, card.id);
    m_positionFrameId.set(row, card.positionFrameId);
    m_xyz.set(row, {card.xyz.x, card.xyz.y, card.xyz.z});
}

void GeomPointTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_xyz.keepRows(rows);
}

void GeomPointTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId cp = m_positionFrameId.get(row); cp > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, cp));
}

} // namespace oor::model
