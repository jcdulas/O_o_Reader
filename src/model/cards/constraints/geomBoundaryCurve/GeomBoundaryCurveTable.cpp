#include "oor/model/cards/constraints/geomBoundaryCurve/GeomBoundaryCurveTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <algorithm>
#include <string>

namespace oor::model
{

namespace
{

constexpr int ENTITY_FIELD = 9;       // the first field of the first continuation line
constexpr int FIRST_ID_FIELD = 10;    // ID1; the list goes on in the next fields, ID8 being in field 17
constexpr int FIRST_UNUSED_FIELD = 4; // fields 4 to 8 of the first line are blank

} // namespace

std::optional<GeomBoundaryCurveCard> GeomBoundaryCurveFormat::read(FieldReader& reader)
{
    GeomBoundaryCurveCard card;
    card.id = reader.requiredInteger(1, "id");
    card.startNode = reader.requiredInteger(2, "startNode");
    card.endNode = reader.requiredInteger(3, "endNode");
    for (int field = FIRST_UNUSED_FIELD; field < ENTITY_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "field " + std::to_string(field + 1) + " of a " +
                                               std::string(reader.cardName()) + " is not used");
            break;
        }
    }
    card.entity =
        readRequiredKeyword<GeomBoundaryCurveEntity>(reader, ENTITY_FIELD, "entity", GEOM_BOUNDARY_CURVE_ENTITY_WORDS);
    // The IDs fill the fields from ID1 on; a blank field is the padding of a continuation line.
    const int last = reader.lastUsedField();
    for (int field = FIRST_ID_FIELD; field <= last && !reader.failed(); ++field)
    {
        if (!reader.isBlank(field))
            card.ids.push_back(reader.requiredInteger(field, "entityIds"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GeomBoundaryCurveFormat::check(const GeomBoundaryCurveCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.startNode, "startNode"))
        return problem;
    if (const auto problem = checkRequiredId(card.endNode, "endNode"))
        return problem;
    for (const base::EntityId id : card.ids)
    {
        if (const auto problem = checkRequiredId(id, "entityIds"))
            return problem;
    }
    // The grid points of the boundary are distinct (format reference: no value may repeat).
    std::vector<base::EntityId> sorted = card.ids;
    std::sort(sorted.begin(), sorted.end());
    if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be unique", "entityIds"};
    return std::nullopt;
}

void GeomBoundaryCurveFormat::write(const GeomBoundaryCurveCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.startNode);
    writer.integer(3, card.endNode);
    writeKeyword(writer, ENTITY_FIELD, GEOM_BOUNDARY_CURVE_ENTITY_WORDS, card.entity);
    int field = FIRST_ID_FIELD;
    for (const base::EntityId id : card.ids)
        writer.integer(field++, id);
}

std::uint32_t GeomBoundaryCurveTable::append(const GeomBoundaryCurveCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void GeomBoundaryCurveTable::store(std::uint32_t row, const GeomBoundaryCurveCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void GeomBoundaryCurveTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void GeomBoundaryCurveTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void GeomBoundaryCurveTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    GeomBoundaryCurveCard& card = m_cards.edit(row);
    visitId(visitor, "startNode", ReferenceTarget::Node, card.startNode);
    visitId(visitor, "endNode", ReferenceTarget::Node, card.endNode);
    for (base::EntityId& id : card.ids)
        visitId(visitor, "entityIds", ReferenceTarget::Node, id);
}

} // namespace oor::model
