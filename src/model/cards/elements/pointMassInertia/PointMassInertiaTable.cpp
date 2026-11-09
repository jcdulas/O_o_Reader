#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"

namespace oor::model
{

std::optional<PointMassInertiaCard> PointMassInertiaFormat::read(FieldReader& reader)
{
    static constexpr const char* NAMES_X[3] = {"offset1", "offset2", "offset3"};
    static constexpr const char* NAMES_I[6] = {"inertia11", "inertia21", "inertia22",
                                               "inertia31", "inertia32", "inertia33"};
    PointMassInertiaCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.frameId = reader.integer(3, "frameId", 0);
    card.mass = reader.real(4, "mass", 0.0);
    for (int index = 0; index < 3; ++index)
        card.offset[static_cast<std::size_t>(index)] = reader.real(5 + index, NAMES_X[index], 0.0);
    // Field 8 is blank: the inertia starts on the continuation line.
    for (int index = 0; index < 6; ++index)
        card.inertia[static_cast<std::size_t>(index)] = reader.real(9 + index, NAMES_I[index], 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PointMassInertiaFormat::check(const PointMassInertiaCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-id", "the grid point ID must be a positive integer", "node"};
    if (card.frameId < PointMassInertiaCard::BASIC_COORDINATES)
        return CardProblem{"card.bad-value", "{field} must be -1, 0 or a coordinate system ID", "frameId"};
    return std::nullopt;
}

void PointMassInertiaFormat::write(const PointMassInertiaCard& card, FieldWriter& writer)
{
    static constexpr int FIRST_INERTIA_FIELD = 9;
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.realUnlessDefault(4, card.mass, 0.0);
    for (int index = 0; index < 3; ++index)
        writer.realUnlessDefault(5 + index, card.offset[static_cast<std::size_t>(index)], 0.0);
    for (int index = 0; index < 6; ++index)
        writer.realUnlessDefault(FIRST_INERTIA_FIELD + index, card.inertia[static_cast<std::size_t>(index)], 0.0);
}

std::uint32_t PointMassInertiaTable::append(const PointMassInertiaCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_frameId.push(card.frameId);
    m_mass.push(card.mass);
    m_offset.push(card.offset);
    m_inertia.push(card.inertia);
    return row;
}

PointMassInertiaCard PointMassInertiaTable::card(std::uint32_t row) const
{
    return PointMassInertiaCard{m_id.get(row),   m_node.get(row),   m_frameId.get(row),
                          m_mass.get(row), m_offset.get(row), m_inertia.get(row)};
}

void PointMassInertiaTable::store(std::uint32_t row, const PointMassInertiaCard& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_frameId.set(row, card.frameId);
    m_mass.set(row, card.mass);
    m_offset.set(row, card.offset);
    m_inertia.set(row, card.inertia);
}

void PointMassInertiaTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_frameId.keepRows(rows);
    m_mass.keepRows(rows);
    m_offset.keepRows(rows);
    m_inertia.keepRows(rows);
}

void PointMassInertiaTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_node.set(row, visitor.visit("node", ReferenceTarget::Node, m_node.get(row)));
    // CID 0 is the basic system and CID -1 means "coordinates in the basic system": neither refers to a card.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
