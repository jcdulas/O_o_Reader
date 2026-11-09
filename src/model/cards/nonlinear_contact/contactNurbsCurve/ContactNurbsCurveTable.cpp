#include "oor/model/cards/nonlinear_contact/contactNurbsCurve/ContactNurbsCurveTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cstdlib>

namespace oor::model
{

std::optional<ContactNurbsCurveCard> ContactNurbsCurveFormat::read(FieldReader& reader)
{
    ContactNurbsCurveCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointCountU = reader.requiredInteger(2, "pointCountU");
    card.orderU = reader.requiredInteger(3, "orderU");
    card.subdivisions = reader.requiredInteger(4, "subdivisions");
    const std::vector<ContactSection> sections =
        splitContactSections(reader, {NURBS_GRID, NURBS_COORD, NURBS_HOMO, NURBS_KNOT});
    card.data = readContactNurbsData(reader, sections);
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty() && section.last > CONTACT_LINE_FIELDS)
            reader.error(
                "card.bad-value",
                "a continuation line of a " + std::string(reader.cardName()) + " starts with GRID, COORD, HOMO or KNOT",
                "nodeId");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactNurbsCurveFormat::check(const ContactNurbsCurveCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.pointCountU == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "pointCountU"};
    if (card.orderU <= 0 || card.subdivisions <= 0)
        return CardProblem{
            "card.bad-value",
            CardProblem::fieldNamed("orderU") + " and " + CardProblem::fieldNamed("subdivisions") + " must be positive",
            card.orderU <= 0 ? "orderU" : "subdivisions"};
    // format reference: NPTU must be negative when the control points are coordinates.
    if (card.data.byCoordinates != (card.pointCountU < 0))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is negative if and only if the control points are given by COORD",
            "pointCountU"};
    const std::int64_t points = std::abs(static_cast<std::int64_t>(card.pointCountU));
    return checkContactNurbsData(card.data, points, 2, points + card.orderU);
}

void ContactNurbsCurveFormat::write(const ContactNurbsCurveCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.pointCountU);
    writer.integer(3, card.orderU);
    writer.integer(4, card.subdivisions);
    writeContactNurbsData(writer, firstFieldOfLine(1), card.data);
}

std::uint32_t ContactNurbsCurveTable::append(const ContactNurbsCurveCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactNurbsCurveTable::store(std::uint32_t row, const ContactNurbsCurveCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactNurbsCurveTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactNurbsCurveTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactNurbsCurveTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitContactIds(m_cards.edit(row).data.grids, visitor, "nodeId", ReferenceTarget::Node);
}

} // namespace oor::model
