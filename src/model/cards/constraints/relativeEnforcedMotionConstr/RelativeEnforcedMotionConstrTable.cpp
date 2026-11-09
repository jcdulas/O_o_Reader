#include "oor/model/cards/constraints/relativeEnforcedMotionConstr/RelativeEnforcedMotionConstrTable.hpp"

#include "oor/base/text/DigitSets.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

std::optional<RelativeEnforcedMotionConstrCard> RelativeEnforcedMotionConstrFormat::read(FieldReader& reader)
{
    RelativeEnforcedMotionConstrCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodes[0] = reader.requiredInteger(2, "node1");
    card.components[0] = base::sortedDigits(reader.integer(3, "components1", 0));
    card.enforced[0] = reader.real(4, "enforcedValue1", 0.0);
    // The second group is there when any of its fields is.
    if (!reader.isBlank(5) || !reader.isBlank(6) || !reader.isBlank(7))
    {
        card.nodes[1] = reader.requiredInteger(5, "node2");
        card.components[1] = base::sortedDigits(reader.integer(6, "components2", 0));
        card.enforced[1] = reader.real(7, "enforcedValue2", 0.0);
    }
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "an " + std::string(reader.cardName()) + " has two groups of three fields",
                     "enforcedValue2");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RelativeEnforcedMotionConstrFormat::check(const RelativeEnforcedMotionConstrCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.nodes[0] <= 0)
        return CardProblem{"card.bad-id", "the grid or scalar point ID must be a positive integer", "node1"};
    if (!base::isComponentSet(card.components[0]))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "components1"};
    if (!std::isfinite(card.enforced[0]))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "enforcedValue1"};
    if (card.hasSecondGroup())
    {
        if (card.nodes[1] < 0)
            return CardProblem{"card.bad-id", "the grid or scalar point ID must be a positive integer", "node2"};
        if (!base::isComponentSet(card.components[1]))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
                "components2"};
        if (!std::isfinite(card.enforced[1]))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "enforcedValue2"};
    }
    else if (card.components[1] != 0 || card.enforced[1] != 0.0)
    {
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("components2") + " and " +
                               CardProblem::fieldNamed("enforcedValue2") + " need a point " +
                               std::string(CardProblem::FIELD),
                           "node2"};
    }
    return std::nullopt;
}

void RelativeEnforcedMotionConstrFormat::write(const RelativeEnforcedMotionConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodes[0]);
    writer.integerUnlessDefault(3, card.components[0], 0);
    writer.realUnlessDefault(4, card.enforced[0], 0.0);
    if (card.hasSecondGroup())
    {
        writer.integer(5, card.nodes[1]);
        writer.integerUnlessDefault(6, card.components[1], 0);
        writer.realUnlessDefault(7, card.enforced[1], 0.0);
    }
}

std::uint32_t RelativeEnforcedMotionConstrTable::append(const RelativeEnforcedMotionConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    m_components.push(card.components);
    m_enforced.push(card.enforced);
    return row;
}

RelativeEnforcedMotionConstrCard RelativeEnforcedMotionConstrTable::card(std::uint32_t row) const
{
    return RelativeEnforcedMotionConstrCard{m_id.get(row), m_nodes.get(row), m_components.get(row),
                                            m_enforced.get(row)};
}

void RelativeEnforcedMotionConstrTable::store(std::uint32_t row, const RelativeEnforcedMotionConstrCard& card)
{
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
    m_components.set(row, card.components);
    m_enforced.set(row, card.enforced);
}

void RelativeEnforcedMotionConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
    m_components.keepRows(rows);
    m_enforced.keepRows(rows);
}

void RelativeEnforcedMotionConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 2> nodes = m_nodes.get(row);
    nodes[0] = visitor.visit("node1", ReferenceTarget::ScalarOrGridPoint, nodes[0]);
    if (nodes[1] > 0)
        nodes[1] = visitor.visit("node2", ReferenceTarget::ScalarOrGridPoint, nodes[1]);
    m_nodes.set(row, nodes);
}

} // namespace oor::model
