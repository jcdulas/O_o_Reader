#include "oor/model/cards/nonlinear_contact/contactTrimCurve/ContactTrimCurveTable.hpp"

#include "oor/model/cards/nonlinear_contact/ContactNurbs.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"

namespace oor::model
{

std::optional<ContactTrimCurveCard> ContactTrimCurveFormat::read(FieldReader& reader)
{
    ContactTrimCurveCard card;
    card.id = reader.requiredInteger(1, "id");
    card.pointCount = reader.requiredInteger(2, "pointCount");
    card.order = reader.requiredInteger(3, "order");
    card.subdivisions = reader.requiredInteger(4, "subdivisions");
    const std::vector<ContactSection> sections = splitContactSections(reader, {NURBS_COORD, NURBS_HOMO, NURBS_KNOT});
    const ContactNurbsData data = readContactNurbsData(reader, sections);
    card.coordinates = data.coordinates;
    card.homogeneous = data.homogeneous;
    card.knots = data.knots;
    for (const ContactSection& section : sections)
    {
        if (section.keyword.empty() && section.last > CONTACT_LINE_FIELDS)
            reader.error(
                "card.bad-value",
                "a continuation line of a " + std::string(reader.cardName()) + " starts with COORD, HOMO or KNOT",
                "coordinates");
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ContactTrimCurveFormat::check(const ContactTrimCurveCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.pointCount <= 0 || card.order <= 0 || card.subdivisions <= 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("pointCount") + ", " + CardProblem::fieldNamed("order") + " and " +
                               CardProblem::fieldNamed("subdivisions") + " must be positive",
                           card.pointCount <= 0 ? "pointCount" : (card.order <= 0 ? "order" : "subdivisions")};
    ContactNurbsData data;
    data.byCoordinates = true;
    data.coordinates = card.coordinates;
    data.homogeneous = card.homogeneous;
    data.knots = card.knots;
    return checkContactNurbsData(data, card.pointCount, 2, static_cast<std::int64_t>(card.pointCount) + card.order);
}

void ContactTrimCurveFormat::write(const ContactTrimCurveCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.pointCount);
    writer.integer(3, card.order);
    writer.integer(4, card.subdivisions);
    ContactNurbsData data;
    data.byCoordinates = true;
    data.coordinates = card.coordinates;
    data.homogeneous = card.homogeneous;
    data.knots = card.knots;
    writeContactNurbsData(writer, firstFieldOfLine(1), data);
}

std::uint32_t ContactTrimCurveTable::append(const ContactTrimCurveCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ContactTrimCurveTable::store(std::uint32_t row, const ContactTrimCurveCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ContactTrimCurveTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ContactTrimCurveTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
