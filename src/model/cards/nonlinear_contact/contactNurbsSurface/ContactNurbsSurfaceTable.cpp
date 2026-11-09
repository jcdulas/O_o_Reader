#include "oor/model/cards/nonlinear_contact/contactNurbsSurface/ContactNurbsSurfaceTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

#include <cstdlib>

namespace oor::model
{

namespace
{

constexpr std::string_view TRIM = "Trimming";

} // namespace

std::optional<ContactNurbsSurfaceCard> ContactNurbsSurfaceFormat::read(FieldReader& reader)
{
    ContactNurbsSurfaceCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointCountU = reader.requiredInteger(2, "pointCountU");
    card.pointCountV = reader.requiredInteger(3, "pointCountV");
    card.orderU = reader.requiredInteger(4, "orderU");
    card.orderV = reader.requiredInteger(5, "orderV");
    card.subdivisionsU = reader.requiredInteger(6, "subdivisionsU");
    card.subdivisionsV = reader.requiredInteger(7, "subdivisionsV");
    const std::vector<ContactSection> sections =
        splitContactSections(reader, {NURBS_GRID, NURBS_COORD, NURBS_HOMO, NURBS_KNOT, TRIM});
    card.data = readContactNurbsData(reader, sections);
    if (const ContactSection* trimCurveIds = findContactSection(sections, TRIM))
        card.trimCurveIds = readContactIds(reader, trimCurveIds->first + 1, trimCurveIds->last, "trimCurveId");
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty() && section.last > CONTACT_LINE_FIELDS)
            reader.error("card.bad-value",
                         "a continuation line of a " + std::string(reader.cardName()) +
                             " starts with GRID, COORD, HOMO, KNOT or TRIM",
                         "nodeId");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactNurbsSurfaceFormat::check(const ContactNurbsSurfaceCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.pointCountU == 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "pointCountU"};
    if (card.pointCountV <= 0 || card.orderU <= 0 || card.orderV <= 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("pointCountV") + ", " + CardProblem::fieldNamed("orderU") + " and " +
                               CardProblem::fieldNamed("orderV") + " must be positive",
                           card.pointCountV <= 0 ? "pointCountV" : (card.orderU <= 0 ? "orderU" : "orderV")};
    if (card.subdivisionsU <= 0 || card.subdivisionsV <= 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("subdivisionsU") + " and " +
                               CardProblem::fieldNamed("subdivisionsV") + " must be positive",
                           card.subdivisionsU <= 0 ? "subdivisionsU" : "subdivisionsV"};
    // NPTU is negative when the control points are coordinates.
    if (card.data.byCoordinates != (card.pointCountU < 0))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " is negative if and only if the control points are given by COORD",
            "pointCountU"};
    const std::int64_t u = std::abs(static_cast<std::int64_t>(card.pointCountU));
    if (auto problem = checkContactNurbsData(card.data, u * card.pointCountV, 3,
                                             (u + card.orderU) + (card.pointCountV + card.orderV)))
        return problem;
    for (const base::EntityId trim : card.trimCurveIds)
    {
        if (trim <= 0)
            return CardProblem{"card.bad-id", "a trimming curve ID must be a positive integer", "trimCurveId"};
    }
    return std::nullopt;
}

void ContactNurbsSurfaceFormat::write(const ContactNurbsSurfaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.pointCountU);
    writer.integer(3, card.pointCountV);
    writer.integer(4, card.orderU);
    writer.integer(5, card.orderV);
    writer.integer(6, card.subdivisionsU);
    writer.integer(7, card.subdivisionsV);
    int line = writeContactNurbsData(writer, firstFieldOfLine(1), card.data);
    if (!card.trimCurveIds.empty())
        writeContactIdList(writer, line, TRIM, card.trimCurveIds);
}

std::uint32_t ContactNurbsSurfaceTable::append(const ContactNurbsSurfaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactNurbsSurfaceTable::store(std::uint32_t row, const ContactNurbsSurfaceCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactNurbsSurfaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactNurbsSurfaceTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ContactNurbsSurfaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ContactNurbsSurfaceCard& card = m_cards.edit(row);
    visitContactIds(card.data.grids, visitor, "nodeId", ReferenceTarget::Node);
    visitContactIds(card.trimCurveIds, visitor, "trimCurveId", ReferenceTarget::TrimmingCurve);
}

} // namespace oor::model
