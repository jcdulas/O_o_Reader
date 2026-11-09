#include "oor/model/cards/constraints/multiPointConstrValue/MultiPointConstrValueTable.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

// The component of the dependent degree of freedom is one digit, unlike the set of digits of an SPCD: the equation has
// one dependent term.
bool isSingleComponent(std::int32_t component) noexcept
{
    return component >= 0 && component <= 6;
}

} // namespace

std::optional<MultiPointConstrValueCard> MultiPointConstrValueFormat::read(FieldReader& reader)
{
    MultiPointConstrValueCard card;
    card.id = reader.requiredInteger(1, "id");
    card.points[0] = reader.requiredInteger(2, "dependentNode1");
    card.components[0] = reader.integer(3, "dependentComponents1", 0);
    card.values[0] = reader.real(4, "rightHandSide1", 0.0);
    // The second equation is there when any of its fields is.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
    {
        card.points[1] = reader.requiredInteger(5, "dependentNode2");
        card.components[1] = reader.integer(6, "dependentComponents2", 0);
        card.values[1] = reader.real(7, "rightHandSide2", 0.0);
    }
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has two equations of three fields",
                     "rightHandSide2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MultiPointConstrValueFormat::check(const MultiPointConstrValueCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.points[0] <= 0)
        return CardProblem{"card.bad-id", "the grid or scalar point ID must be a positive integer", "dependentNode1"};
    if (!isSingleComponent(card.components[0]))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar point",
            "dependentComponents1"};
    if (!std::isfinite(card.values[0]))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "rightHandSide1"};
    if (card.hasSecondEquation())
    {
        if (card.points[1] < 0)
            return CardProblem{"card.bad-id", "the grid or scalar point ID must be a positive integer",
                               "dependentNode2"};
        if (!isSingleComponent(card.components[1]))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar point",
                "dependentComponents2"};
        if (!std::isfinite(card.values[1]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "rightHandSide2"};
    }
    else if (card.components[1] != 0 || card.values[1] != 0.0)
    {
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("dependentComponents2") + " and " +
                               CardProblem::fieldNamed("rightHandSide2") + " need a point " +
                               std::string(CardProblem::FIELD),
                           "dependentNode2"};
    }
    return std::nullopt;
}

void MultiPointConstrValueFormat::write(const MultiPointConstrValueCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.points[0]);
    writer.integerUnlessDefault(3, card.components[0], 0);
    writer.realUnlessDefault(4, card.values[0], 0.0);
    if (card.hasSecondEquation())
    {
        writer.integer(5, card.points[1]);
        writer.integerUnlessDefault(6, card.components[1], 0);
        writer.realUnlessDefault(7, card.values[1], 0.0);
    }
}

std::uint32_t MultiPointConstrValueTable::append(const MultiPointConstrValueCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_points.push(card.points);
    m_components.push(card.components);
    m_values.push(card.values);
    return row;
}

MultiPointConstrValueCard MultiPointConstrValueTable::card(std::uint32_t row) const
{
    return MultiPointConstrValueCard{m_id.get(row), m_points.get(row), m_components.get(row), m_values.get(row)};
}

void MultiPointConstrValueTable::store(std::uint32_t row, const MultiPointConstrValueCard& card)
{
    m_id.set(row, card.id);
    m_points.set(row, card.points);
    m_components.set(row, card.components);
    m_values.set(row, card.values);
}

void MultiPointConstrValueTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_points.keepRows(rows);
    m_components.keepRows(rows);
    m_values.keepRows(rows);
}

void MultiPointConstrValueTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 2> points = m_points.get(row);
    points[0] = visitor.visit("dependentNode1", ReferenceTarget::ScalarOrGridPoint, points[0]);
    if (points[1] > 0)
        points[1] = visitor.visit("dependentNode2", ReferenceTarget::ScalarOrGridPoint, points[1]);
    m_points.set(row, points);
}

} // namespace oor::model
