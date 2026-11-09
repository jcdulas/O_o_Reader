#include "oor/model/cards/rigid_elements/rigidSpline/RigidSplineTable.hpp"

#include "oor/base/text/DigitSets.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIRST_POINT_FIELD = 3;
constexpr double DEFAULT_RATIO = 0.1;

// The field of the point of position `index` (0 is G1): G1 is in field 3, then a point and its components take two
// fields each, so that G2 is in field 4, its components in field 5, G3 in field 6, ...
int pointField(std::size_t index)
{
    return index == 0 ? FIRST_POINT_FIELD : 2 + 2 * static_cast<int>(index);
}

} // namespace

std::optional<RigidSplineCard> RigidSplineFormat::read(FieldReader& reader)
{
    RigidSplineCard card;
    card.id = reader.requiredInteger(1, "id");
    card.ratio = reader.real(2, "diameterRatio", DEFAULT_RATIO);
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; pointField(index) <= last && !reader.failed(); ++index)
    {
        const int field = pointField(index);
        if (index > 0 && reader.isBlank(field) && reader.isBlank(field + 1))
            continue; // padding
        const std::string number = std::to_string(index + 1);
        const std::string pointName = "node" + number;
        const std::string componentName = "components" + number;
        DofPair point;
        point.node = reader.requiredInteger(field, pointName.c_str());
        if (index > 0)
            point.components = base::sortedDigits(reader.integer(field + 1, componentName.c_str(), 0));
        card.points.push_back(point);
    }
    // the last field of the card is a point, not a components field.
    if (!reader.failed() && last >= 5 && last % 2 == 1)
        reader.error("card.bad-value", "the last field of the element must be a grid point, not a components field",
                     "components");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidSplineFormat::check(const RigidSplineCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (!(card.ratio > 0.0) || !std::isfinite(card.ratio))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "diameterRatio"};
    if (card.points.size() < 2)
        return CardProblem{"card.missing-field", "the element needs at least two grid points", "node2"};
    for (const DofPair& point : card.points)
    {
        if (point.node <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", "node1"};
        if (!base::isComponentSet(point.components))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
                "components2"};
    }
    if (card.points.front().components != 0)
        return CardProblem{"card.bad-value", "the first grid point is independent: it has no components field",
                           "components1"};
    if (card.points.back().components != 0)
        return CardProblem{"card.bad-value", "the last grid point is independent: it has no components field",
                           "components"};
    return std::nullopt;
}

void RigidSplineFormat::write(const RigidSplineCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.realUnlessDefault(2, card.ratio, DEFAULT_RATIO);
    for (std::size_t index = 0; index < card.points.size(); ++index)
    {
        const int field = pointField(index);
        writer.integer(field, card.points[index].node);
        // The first and the last point have no components (they are independent).
        if (index > 0 && index + 1 < card.points.size())
            writer.integerUnlessDefault(field + 1, card.points[index].components, 0);
    }
}

std::uint32_t RigidSplineTable::append(const RigidSplineCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_diameterRatio.push(card.ratio);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.points, nodes, components);
    m_node.push(nodes);
    m_component.push(components);
    return row;
}

RigidSplineCard RigidSplineTable::card(std::uint32_t row) const
{
    RigidSplineCard card;
    card.id = m_id.get(row);
    card.ratio = m_diameterRatio.get(row);
    card.points = joinDofPairs(m_node.get(row), m_component.get(row));
    return card;
}

void RigidSplineTable::store(std::uint32_t row, const RigidSplineCard& card)
{
    m_id.set(row, card.id);
    m_diameterRatio.set(row, card.ratio);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    splitDofPairs(card.points, nodes, components);
    m_node.set(row, nodes);
    m_component.set(row, components);
}

void RigidSplineTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_diameterRatio.keepRows(rows);
    m_node.keepRows(rows);
    m_component.keepRows(rows);
}

void RigidSplineTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitIdList(m_node, row, visitor, "node1", ReferenceTarget::Node);
}

} // namespace oor::model
