#include "oor/model/cards/nonlinear_contact/contactBezierSurface/ContactBezierSurfaceTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_GRID_FIELD = 6;

} // namespace

std::optional<ContactBezierSurfaceCard> ContactBezierSurfaceFormat::read(FieldReader& reader)
{
    ContactBezierSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointCount1 = reader.requiredInteger(2, "pointCount1");
    card.pointCount2 = reader.requiredInteger(3, "pointCount2");
    card.subdivisions1 = reader.requiredInteger(4, "subdivisions1");
    card.subdivisions2 = reader.requiredInteger(5, "subdivisions2");
    card.nodes = readContactIds(reader, FIRST_GRID_FIELD, reader.lastUsedField(), "node");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactBezierSurfaceFormat::check(const ContactBezierSurfaceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.pointCount1 <= 0 || card.pointCount2 <= 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("pointCount1") + " and " + CardProblem::fieldNamed("pointCount2") +
                               " must be positive",
                           card.pointCount1 <= 0 ? "pointCount1" : "pointCount2"};
    if (card.subdivisions1 <= 0 || card.subdivisions2 <= 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("subdivisions1") + " and " +
                               CardProblem::fieldNamed("subdivisions2") + " must be positive",
                           card.subdivisions1 <= 0 ? "subdivisions1" : "subdivisions2"};
    if (static_cast<std::int64_t>(card.nodes.size()) != static_cast<std::int64_t>(card.pointCount1) * card.pointCount2)
        return CardProblem{"card.bad-value",
                           "the surface needs " + CardProblem::fieldNamed("pointCount1") + " x " +
                               CardProblem::fieldNamed("pointCount2") + " grid points",
                           "node"};
    for (const base::EntityId grid : card.nodes)
    {
        if (grid <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "node"};
    }
    return std::nullopt;
}

void ContactBezierSurfaceFormat::write(const ContactBezierSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.pointCount1);
    writer.integer(3, card.pointCount2);
    writer.integer(4, card.subdivisions1);
    writer.integer(5, card.subdivisions2);
    for (std::size_t index = 0; index < card.nodes.size(); ++index)
        writer.integer(FIRST_GRID_FIELD + static_cast<int>(index), card.nodes[index]);
}

std::uint32_t ContactBezierSurfaceTable::append(const ContactBezierSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_pointCount1.push(card.pointCount1);
    m_pointCount2.push(card.pointCount2);
    m_subdivisions1.push(card.subdivisions1);
    m_subdivisions2.push(card.subdivisions2);
    m_nodes.push(card.nodes);
    return row;
}

ContactBezierSurfaceCard ContactBezierSurfaceTable::card(std::uint32_t row) const
{
    ContactBezierSurfaceCard card;
    card.id = m_id.get(row);
    card.pointCount1 = m_pointCount1.get(row);
    card.pointCount2 = m_pointCount2.get(row);
    card.subdivisions1 = m_subdivisions1.get(row);
    card.subdivisions2 = m_subdivisions2.get(row);
    const auto nodes = m_nodes.get(row);
    card.nodes.assign(nodes.begin(), nodes.end());
    return card;
}

void ContactBezierSurfaceTable::store(std::uint32_t row, const ContactBezierSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_pointCount1.set(row, card.pointCount1);
    m_pointCount2.set(row, card.pointCount2);
    m_subdivisions1.set(row, card.subdivisions1);
    m_subdivisions2.set(row, card.subdivisions2);
    m_nodes.set(row, card.nodes);
}

void ContactBezierSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_pointCount1.keepRows(rows);
    m_pointCount2.keepRows(rows);
    m_subdivisions1.keepRows(rows);
    m_subdivisions2.keepRows(rows);
    m_nodes.keepRows(rows);
}

void ContactBezierSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIdColumn(m_nodes, row, visitor, "node", ReferenceTarget::Node);
}

} // namespace oor::model
