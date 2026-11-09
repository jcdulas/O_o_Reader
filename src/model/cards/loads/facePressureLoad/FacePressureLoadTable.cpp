#include "oor/model/cards/loads/facePressureLoad/FacePressureLoadTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

constexpr double BLANK_PRESSURE = std::numeric_limits<double>::quiet_NaN();

struct NamedLineDirection
{
    const char* name;
    LineLoadDirection direction;
};
constexpr NamedLineDirection LINE_DIRECTIONS[] = {{"Norm", LineLoadDirection::Normal},
                                                  {"XDirection", LineLoadDirection::X},
                                                  {"YDirection", LineLoadDirection::Y},
                                                  {"ZDirection", LineLoadDirection::Z},
                                                  {"Tangent", LineLoadDirection::Tangent}};

const char* directionName(LineLoadDirection direction)
{
    for (const NamedLineDirection& entry : LINE_DIRECTIONS)
    {
        if (entry.direction == direction)
            return entry.name;
    }
    return "Norm";
}

bool hasVector(const FacePressureLoadCard& card)
{
    return card.direction[0] != 0.0 || card.direction[1] != 0.0 || card.direction[2] != 0.0;
}

} // namespace

std::optional<FacePressureLoadCard> FacePressureLoadFormat::read(FieldReader& reader)
{
    FacePressureLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    card.elemId = reader.requiredInteger(2, "elemId");
    card.pressures[0] = reader.real(3, "pressure1", 0.0);
    std::optional<double> others[3];
    for (int index = 0; index < 3; ++index)
        others[index] =
            reader.optionalReal(4 + index, index == 0 ? "pressure2" : (index == 1 ? "pressure3" : "pressure4"));

    // Field 7 is G1 (any element) or "Through" (a range of shell elements, with EID2 in field 8).
    if ((reader.text(7) == "Through"))
    {
        card.elemId2 = reader.requiredInteger(8, "elemId2");
    }
    else
    {
        card.node1 = reader.integer(7, "node1", 0);
        card.node3Or4 = reader.integer(8, "node3/node4", 0);
    }

    card.frameId = reader.integer(9, "frameId", 0);
    card.direction[0] = reader.real(10, "directionX", 0.0);
    card.direction[1] = reader.real(11, "directionY", 0.0);
    card.direction[2] = reader.real(12, "directionZ", 0.0);
    if (const std::string_view surfaceOrLine = reader.text(13); !surfaceOrLine.empty())
    {
        if ((surfaceOrLine == "Line"))
            card.kind = PressureKind::Line;
        else if (!(surfaceOrLine == "Surface"))
            reader.error("card.bad-value",
                         std::string("surfaceOrLine") + " must be SURF, LINE or blank, found \"" +
                             std::string(surfaceOrLine) + "\"",
                         "surfaceOrLine");
    }
    if (const std::string_view lineLoadDirection = reader.text(14); !lineLoadDirection.empty())
    {
        bool found = false;
        for (const NamedLineDirection& entry : LINE_DIRECTIONS)
        {
            if ((lineLoadDirection == entry.name))
            {
                card.lineDirection = entry.direction;
                found = true;
            }
        }
        if (!found)
            reader.error("card.bad-value",
                         std::string("lineLoadDirection") + " must be X, Y, Z, TANG, NORM or blank, found \"" +
                             std::string(lineLoadDirection) + "\"",
                         "lineLoadDirection");
    }

    // format reference: P2, P3 and P4 default to P1; for a line load a blank stays blank (see FacePressureLoadCard).
    for (int index = 0; index < 3; ++index)
    {
        const double blank = card.kind == PressureKind::Surface ? card.pressures[0] : BLANK_PRESSURE;
        card.pressures[static_cast<std::size_t>(index) + 1] = others[index].value_or(blank);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FacePressureLoadFormat::check(const FacePressureLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.elemId <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "elemId"};
    if (card.elemId2 < 0 || (card.elemId2 != 0 && card.elemId2 <= card.elemId))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " of a THRU range must be above " + CardProblem::fieldNamed("elemId"),
            "elemId2"};
    if (card.node1 < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "node1"};
    if (card.node3Or4 < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "node3/node4"};
    if (card.isRange() && (card.node1 != 0 || card.node3Or4 != 0))
        return CardProblem{"card.bad-value",
                           "the THRU form has no " + CardProblem::fieldNamed("node1") + " and " +
                               CardProblem::fieldNamed("node3/node4"),
                           "elemId2"};
    if (card.frameId < 0)
        return CardProblem{"card.bad-value", "the coordinate system ID cannot be negative", "frameId"};
    if (std::isnan(card.pressures[0]))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a number", "pressure1"};
    if (card.kind == PressureKind::Surface)
    {
        for (const double pressure : card.pressures)
        {
            if (std::isnan(pressure))
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("pressure3") +
                                       " and " + CardProblem::fieldNamed("pressure4") +
                                       " of a surface load must be numbers",
                                   "pressure2"};
        }
    }
    // a line load has its direction from the vector or from LDIR, not from both.
    if (card.kind == PressureKind::Line && card.lineDirection != LineLoadDirection::Normal && hasVector(card))
        return CardProblem{"card.bad-value",
                           "a line load has a vector (" + CardProblem::fieldNamed("directionX") + ", " +
                               CardProblem::fieldNamed("directionY") + ", " + CardProblem::fieldNamed("directionZ") +
                               ") or " + std::string(CardProblem::FIELD) + ", not both",
                           "lineLoadDirection"};
    return std::nullopt;
}

void FacePressureLoadFormat::write(const FacePressureLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.elemId);
    writer.realUnlessDefault(3, card.pressures[0], 0.0);
    // P2 to P4 depend on P1 (surface loads): always written.
    for (std::size_t index = 1; index < 4; ++index)
    {
        if (!std::isnan(card.pressures[index]))
            writer.real(3 + static_cast<int>(index), card.pressures[index]);
    }
    if (card.isRange())
    {
        writer.text(7, "Through");
        writer.integer(8, card.elemId2);
    }
    else
    {
        writer.integerUnlessDefault(7, card.node1, 0);
        writer.integerUnlessDefault(8, card.node3Or4, 0);
    }
    writer.integerUnlessDefault(9, card.frameId, 0);
    writer.realUnlessDefault(10, card.direction[0], 0.0);
    writer.realUnlessDefault(11, card.direction[1], 0.0);
    writer.realUnlessDefault(12, card.direction[2], 0.0);
    writer.textUnlessDefault(13, card.kind == PressureKind::Line ? "Line" : "Surface", "Surface");
    writer.textUnlessDefault(14, directionName(card.lineDirection), "Norm");
}

std::uint32_t FacePressureLoadTable::append(const FacePressureLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_elemId.push(card.elemId);
    m_elemId2.push(card.elemId2);
    m_pressures.push(card.pressures);
    m_node1.push(card.node1);
    m_node3Or4.push(card.node3Or4);
    m_frameId.push(card.frameId);
    m_direction.push(card.direction);
    m_kind.push(static_cast<std::uint8_t>(card.kind));
    m_lineDirection.push(static_cast<std::uint8_t>(card.lineDirection));
    return row;
}

FacePressureLoadCard FacePressureLoadTable::card(std::uint32_t row) const
{
    FacePressureLoadCard card;
    card.id = m_id.get(row);
    card.elemId = m_elemId.get(row);
    card.elemId2 = m_elemId2.get(row);
    card.pressures = m_pressures.get(row);
    card.node1 = m_node1.get(row);
    card.node3Or4 = m_node3Or4.get(row);
    card.frameId = m_frameId.get(row);
    card.direction = m_direction.get(row);
    card.kind = static_cast<PressureKind>(m_kind.get(row));
    card.lineDirection = static_cast<LineLoadDirection>(m_lineDirection.get(row));
    return card;
}

void FacePressureLoadTable::store(std::uint32_t row, const FacePressureLoadCard& card)
{
    m_id.set(row, card.id);
    m_elemId.set(row, card.elemId);
    m_elemId2.set(row, card.elemId2);
    m_pressures.set(row, card.pressures);
    m_node1.set(row, card.node1);
    m_node3Or4.set(row, card.node3Or4);
    m_frameId.set(row, card.frameId);
    m_direction.set(row, card.direction);
    m_kind.set(row, static_cast<std::uint8_t>(card.kind));
    m_lineDirection.set(row, static_cast<std::uint8_t>(card.lineDirection));
}

void FacePressureLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_elemId.keepRows(rows);
    m_elemId2.keepRows(rows);
    m_pressures.keepRows(rows);
    m_node1.keepRows(rows);
    m_node3Or4.keepRows(rows);
    m_frameId.keepRows(rows);
    m_direction.keepRows(rows);
    m_kind.keepRows(rows);
    m_lineDirection.keepRows(rows);
}

void FacePressureLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The two ends of a THRU range are elements that exist; the ones between them keep their IDs (renumbering a model
    // with ranges must renumber the elements in order, as for the nodes of a THRU list).
    m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::Element, m_elemId.get(row)));
    if (const base::EntityId elemId2 = m_elemId2.get(row); elemId2 > 0)
        m_elemId2.set(row, visitor.visit("elemId2", ReferenceTarget::Element, elemId2));
    if (const base::EntityId node1 = m_node1.get(row); node1 > 0)
        m_node1.set(row, visitor.visit("node1", ReferenceTarget::Node, node1));
    if (const base::EntityId node3Or4 = m_node3Or4.get(row); node3Or4 > 0)
        m_node3Or4.set(row, visitor.visit("node3", ReferenceTarget::Node, node3Or4));
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
