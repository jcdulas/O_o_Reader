#include "oor/model/cards/elements/beam3/Beam3Table.hpp"

#include <variant>

namespace oor::model
{

namespace
{

// The three reals that start at `first`, blank ones 0.
std::array<double, 3> readTriple(FieldReader& reader, int first, const char* const (&names)[3])
{
    std::array<double, 3> values = {};
    for (std::size_t index = 0; index < 3; ++index)
        values[index] = reader.real(first + static_cast<int>(index), names[index], 0.0);
    return values;
}

// Writes a triple of reals, leaving the zeros blank.
void writeTriple(FieldWriter& writer, int first, const std::array<double, 3>& values)
{
    for (std::size_t index = 0; index < 3; ++index)
        writer.realUnlessDefault(first + static_cast<int>(index), values[index], 0.0);
}

} // namespace

std::optional<Beam3Card> Beam3Format::read(FieldReader& reader)
{
    static constexpr const char* NAMES_A[3] = {"offset1A", "offset2A", "offset3A"};
    static constexpr const char* NAMES_B[3] = {"offset1B", "offset2B", "offset3B"};
    static constexpr const char* NAMES_C[3] = {"offset1C", "offset2C", "offset3C"};
    static constexpr const char* NAMES_TWIST[3] = {"twistA", "twistB", "twistC"};
    Beam3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.propId = reader.requiredInteger(2, "propId");
    card.nodeA = reader.requiredInteger(3, "nodeA");
    card.nodeB = reader.requiredInteger(4, "nodeB");
    card.nodeC = reader.integer(5, "nodeC", 0);
    // Field 6 is GO (an integer) or X1 (a real); X2 and X3 are only read for the vector form.
    const auto orientation = reader.integerOrReal(6, "orientationNode/orientation1");
    if (orientation && std::holds_alternative<std::int32_t>(*orientation))
    {
        card.orientationNode = std::get<std::int32_t>(*orientation);
        if (card.orientationNode <= 0)
            reader.error("card.bad-value", std::string("orientationNode") + " must be a positive grid point ID",
                         "orientationNode");
    }
    else
    {
        card.orientation[0] = orientation ? std::get<double>(*orientation) : 0.0;
        card.orientation[1] = reader.real(7, "orientation2", 0.0);
        card.orientation[2] = reader.real(8, "orientation3", 0.0);
    }
    card.offsetA = readTriple(reader, 9, NAMES_A);
    card.offsetB = readTriple(reader, 12, NAMES_B);
    card.offsetC = readTriple(reader, 15, NAMES_C);
    card.twist = readTriple(reader, 18, NAMES_TWIST);
    card.warpingPointA = reader.integer(21, "warpingPointA", 0);
    card.warpingPointB = reader.integer(22, "warpingPointB", 0);
    card.warpingPointC = reader.integer(23, "warpingPointC", 0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> Beam3Format::check(const Beam3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.propId <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "propId"};
    if (card.nodeA <= 0 || card.nodeB <= 0)
        return CardProblem{"card.bad-id", "{field} and {field:nodeB} must be positive grid point IDs", "nodeA"};
    if (card.nodeC < 0)
        return CardProblem{"card.bad-id", "{field} must be 0 (blank) or a grid point ID", "nodeC"};
    if (card.nodeA == card.nodeB || (card.nodeC != 0 && (card.nodeC == card.nodeA || card.nodeC == card.nodeB)))
        return CardProblem{"card.duplicate-node", "{field:nodeA}, {field} and {field:nodeC} must be different",
                           "nodeB"};
    if (card.orientationNode < 0)
        return CardProblem{"card.bad-value", "{field} must be a positive grid point ID", "orientationNode"};
    if (card.orientationNode != 0 && card.hasVector())
        return CardProblem{"card.exclusive-fields",
                           "the orientation is a grid point {field} or a vector {field:orientation1} "
                           "{field:orientation2} {field:orientation3}, not both",
                           "orientationNode"};
    if (card.orientationNode == 0 && !card.hasVector())
        return CardProblem{"card.missing-field",
                           "the orientation (a grid point {field} or a vector {field:orientation1} "
                           "{field:orientation2} {field:orientation3}) is required",
                           "orientationNode"};
    if (card.orientationNode != 0 && (card.orientationNode == card.nodeA || card.orientationNode == card.nodeB ||
                                      card.orientationNode == card.nodeC))
        return CardProblem{"card.duplicate-node", "{field} cannot be {field:nodeA}, {field:nodeB} or {field:nodeC}",
                           "orientationNode"};
    if (card.warpingPointA < 0 || card.warpingPointB < 0 || card.warpingPointC < 0)
        return CardProblem{"card.bad-id",
                           "{field}, {field:warpingPointB} and {field:warpingPointC} must be 0 (blank) or point IDs",
                           "warpingPointA"};
    return std::nullopt;
}

void Beam3Format::write(const Beam3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.propId);
    writer.integer(3, card.nodeA);
    writer.integer(4, card.nodeB);
    writer.integerUnlessDefault(5, card.nodeC, 0);
    if (card.orientationNode != 0)
    {
        writer.integer(6, card.orientationNode);
    }
    else
    {
        // The orientation is required, so the vector is there; the three components are written together.
        writer.real(6, card.orientation[0]);
        writer.real(7, card.orientation[1]);
        writer.real(8, card.orientation[2]);
    }
    writeTriple(writer, 9, card.offsetA);
    writeTriple(writer, 12, card.offsetB);
    writeTriple(writer, 15, card.offsetC);
    writeTriple(writer, 18, card.twist);
    writer.integerUnlessDefault(21, card.warpingPointA, 0);
    writer.integerUnlessDefault(22, card.warpingPointB, 0);
    writer.integerUnlessDefault(23, card.warpingPointC, 0);
}

std::uint32_t Beam3Table::append(const Beam3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_propId.push(card.propId);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_nodeC.push(card.nodeC);
    m_orientationNode.push(card.orientationNode);
    m_orientation.push(card.orientation);
    m_offsetA.push(card.offsetA);
    m_offsetB.push(card.offsetB);
    m_offsetC.push(card.offsetC);
    m_twist.push(card.twist);
    m_warpingPointA.push(card.warpingPointA);
    m_warpingPointB.push(card.warpingPointB);
    m_warpingPointC.push(card.warpingPointC);
    return row;
}

Beam3Card Beam3Table::card(std::uint32_t row) const
{
    return Beam3Card{m_id.get(row),
                         m_propId.get(row),
                         m_nodeA.get(row),
                         m_nodeB.get(row),
                         m_nodeC.get(row),
                         m_orientationNode.get(row),
                         m_orientation.get(row),
                         m_offsetA.get(row),
                         m_offsetB.get(row),
                         m_offsetC.get(row),
                         m_twist.get(row),
                         m_warpingPointA.get(row),
                         m_warpingPointB.get(row),
                         m_warpingPointC.get(row)};
}

void Beam3Table::store(std::uint32_t row, const Beam3Card& card)
{
    m_id.set(row, card.id);
    m_propId.set(row, card.propId);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_nodeC.set(row, card.nodeC);
    m_orientationNode.set(row, card.orientationNode);
    m_orientation.set(row, card.orientation);
    m_offsetA.set(row, card.offsetA);
    m_offsetB.set(row, card.offsetB);
    m_offsetC.set(row, card.offsetC);
    m_twist.set(row, card.twist);
    m_warpingPointA.set(row, card.warpingPointA);
    m_warpingPointB.set(row, card.warpingPointB);
    m_warpingPointC.set(row, card.warpingPointC);
}

void Beam3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_propId.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_nodeC.keepRows(rows);
    m_orientationNode.keepRows(rows);
    m_orientation.keepRows(rows);
    m_offsetA.keepRows(rows);
    m_offsetB.keepRows(rows);
    m_offsetC.keepRows(rows);
    m_twist.keepRows(rows);
    m_warpingPointA.keepRows(rows);
    m_warpingPointB.keepRows(rows);
    m_warpingPointC.keepRows(rows);
}

void Beam3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_propId.set(row, visitor.visit("propId", ReferenceTarget::Property, m_propId.get(row)));
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
    if (const base::EntityId nodeC = m_nodeC.get(row); nodeC > 0)
        m_nodeC.set(row, visitor.visit("nodeC", ReferenceTarget::Node, nodeC));
    if (const base::EntityId orientationNode = m_orientationNode.get(row); orientationNode > 0)
        m_orientationNode.set(row, visitor.visit("orientationNode", ReferenceTarget::Node, orientationNode));
    // SA, SB and SC are the grid or scalar points of the warping degrees of freedom.
    if (const base::EntityId warpingPointA = m_warpingPointA.get(row); warpingPointA > 0)
        m_warpingPointA.set(row, visitor.visit("warpingPointA", ReferenceTarget::ScalarOrGridPoint, warpingPointA));
    if (const base::EntityId warpingPointB = m_warpingPointB.get(row); warpingPointB > 0)
        m_warpingPointB.set(row, visitor.visit("warpingPointB", ReferenceTarget::ScalarOrGridPoint, warpingPointB));
    if (const base::EntityId warpingPointC = m_warpingPointC.get(row); warpingPointC > 0)
        m_warpingPointC.set(row, visitor.visit("warpingPointC", ReferenceTarget::ScalarOrGridPoint, warpingPointC));
}

} // namespace oor::model
