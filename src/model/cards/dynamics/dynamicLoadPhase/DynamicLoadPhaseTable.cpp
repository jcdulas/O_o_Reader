#include "oor/model/cards/dynamics/dynamicLoadPhase/DynamicLoadPhaseTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<DynamicLoadPhaseCard> DynamicLoadPhaseFormat::read(FieldReader& reader)
{
    DynamicLoadPhaseCard card;
    card.id = reader.requiredInteger(1, "id");
    card.points[0] = reader.requiredInteger(2, "point1");
    card.components[0] = reader.integer(3, "components1", 0);
    card.phaseAngles[0] = reader.requiredReal(4, "phaseAngle1");
    // The second group is there when any of its fields is.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
    {
        card.points[1] = reader.requiredInteger(5, "point2");
        card.components[1] = reader.integer(6, "components2", 0);
        card.phaseAngles[1] = reader.requiredReal(7, "phaseAngle2");
    }
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has at most two groups of a point, its component and a phase angle",
                     "phaseAngle2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DynamicLoadPhaseFormat::check(const DynamicLoadPhaseCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.points[0] <= 0)
        return CardProblem{"card.bad-id", "the point ID must be a positive integer", "point1"};
    if (card.components[0] < 0 || card.components[0] > 6)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 1 to 6 for a grid point, 0 or blank for a scalar point",
            "components1"};
    if (!std::isfinite(card.phaseAngles[0]))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "phaseAngle1"};
    if (card.hasSecondGroup())
    {
        if (card.points[1] < 0)
            return CardProblem{"card.bad-id", "the point ID must be a positive integer", "point2"};
        if (card.components[1] < 0 || card.components[1] > 6)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 1 to 6 for a grid point, 0 or blank for a scalar point",
                "components2"};
        if (!std::isfinite(card.phaseAngles[1]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "phaseAngle2"};
    }
    else if (card.components[1] != 0 || card.phaseAngles[1] != 0.0)
    {
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("components2") + " and " + CardProblem::fieldNamed("phaseAngle2") +
                               " need a point " + std::string(CardProblem::FIELD),
                           "point2"};
    }
    return std::nullopt;
}

void DynamicLoadPhaseFormat::write(const DynamicLoadPhaseCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.points[0]);
    writer.integerUnlessDefault(3, card.components[0], 0);
    writer.real(4, card.phaseAngles[0]);
    if (card.hasSecondGroup())
    {
        writer.integer(5, card.points[1]);
        writer.integerUnlessDefault(6, card.components[1], 0);
        writer.real(7, card.phaseAngles[1]);
    }
}

std::uint32_t DynamicLoadPhaseTable::append(const DynamicLoadPhaseCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_points.push(card.points);
    m_components.push(card.components);
    m_phaseAngles.push(card.phaseAngles);
    return row;
}

DynamicLoadPhaseCard DynamicLoadPhaseTable::card(std::uint32_t row) const
{
    return DynamicLoadPhaseCard{m_id.get(row), m_points.get(row), m_components.get(row), m_phaseAngles.get(row)};
}

void DynamicLoadPhaseTable::store(std::uint32_t row, const DynamicLoadPhaseCard& card)
{
    m_id.set(row, card.id);
    m_points.set(row, card.points);
    m_components.set(row, card.components);
    m_phaseAngles.set(row, card.phaseAngles);
}

void DynamicLoadPhaseTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_points.keepRows(rows);
    m_components.keepRows(rows);
    m_phaseAngles.keepRows(rows);
}

void DynamicLoadPhaseTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 2> points = m_points.get(row);
    points[0] = visitor.visit("point1", ReferenceTarget::Node, points[0]);
    if (points[1] > 0)
        points[1] = visitor.visit("point2", ReferenceTarget::Node, points[1]);
    m_points.set(row, points);
}

} // namespace oor::model
