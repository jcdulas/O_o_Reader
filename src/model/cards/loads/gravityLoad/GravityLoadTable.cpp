#include "oor/model/cards/loads/gravityLoad/GravityLoadTable.hpp"

namespace oor::model
{

std::optional<GravityLoadCard> GravityLoadFormat::read(FieldReader& reader)
{
    GravityLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.frameId = reader.integer(2, "frameId", 0);
    card.magnitude = reader.requiredReal(3, "magnitude");
    card.direction[0] = reader.real(4, "directionX", 0.0);
    card.direction[1] = reader.real(5, "directionY", 0.0);
    card.direction[2] = reader.real(6, "directionZ", 0.0);
    card.mainBulkFlag = reader.integer(7, "mainBulkFlag", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GravityLoadFormat::check(const GravityLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-value", "the coordinate system ID cannot be negative", "frameId"};
    // format reference: at least one component of the vector is not zero.
    if (card.direction[0] == 0.0 && card.direction[1] == 0.0 && card.direction[2] == 0.0)
        return CardProblem{"card.bad-value",
                           "the acceleration vector (" + std::string(CardProblem::FIELD) + ", " +
                               CardProblem::fieldNamed("directionY") + ", " + CardProblem::fieldNamed("directionZ") +
                               ") must not be zero",
                           "directionX"};
    return std::nullopt;
}

void GravityLoadFormat::write(const GravityLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.frameId, 0);
    writer.real(3, card.magnitude);
    writer.realUnlessDefault(4, card.direction[0], 0.0);
    writer.realUnlessDefault(5, card.direction[1], 0.0);
    writer.realUnlessDefault(6, card.direction[2], 0.0);
    writer.integerUnlessDefault(7, card.mainBulkFlag, 0);
}

std::uint32_t GravityLoadTable::append(const GravityLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_frameId.push(card.frameId);
    m_magnitude.push(card.magnitude);
    m_direction.push(card.direction);
    m_mainBulkFlag.push(card.mainBulkFlag);
    return row;
}

GravityLoadCard GravityLoadTable::card(std::uint32_t row) const
{
    return GravityLoadCard{m_id.get(row), m_frameId.get(row), m_magnitude.get(row), m_direction.get(row),
                           m_mainBulkFlag.get(row)};
}

void GravityLoadTable::store(std::uint32_t row, const GravityLoadCard& card)
{
    m_id.set(row, card.id);
    m_frameId.set(row, card.frameId);
    m_magnitude.set(row, card.magnitude);
    m_direction.set(row, card.direction);
    m_mainBulkFlag.set(row, card.mainBulkFlag);
}

void GravityLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_frameId.keepRows(rows);
    m_magnitude.keepRows(rows);
    m_direction.keepRows(rows);
    m_mainBulkFlag.keepRows(rows);
}

void GravityLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
