#include "oor/model/cards/dynamics/transientInitialCondition/TransientInitialConditionTable.hpp"

#include <cmath>

namespace oor::model
{

std::optional<TransientInitialConditionCard> TransientInitialConditionFormat::read(FieldReader& reader)
{
    TransientInitialConditionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "node");
    card.components = reader.integer(3, "components", 0);
    card.initialDisplacement = reader.real(4, "initialDisplacement", 0.0);
    card.initialVelocity = reader.real(5, "initialVelocity", 0.0);
    if (reader.lastUsedField() > 5)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("initialVelocity"),
                     "initialVelocity");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransientInitialConditionFormat::check(const TransientInitialConditionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer", "node"};
    if (card.components < TransientInitialConditionCard::MODAL || card.components > 6)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " must be a component from 1 to 6, 0 for a scalar point or -1 for a modal coordinate",
                           "components"};
    if (!std::isfinite(card.initialDisplacement) || !std::isfinite(card.initialVelocity))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("initialVelocity") +
                               " must be finite numbers",
                           "initialDisplacement"};
    return std::nullopt;
}

void TransientInitialConditionFormat::write(const TransientInitialConditionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.integerUnlessDefault(3, card.components, 0);
    writer.realUnlessDefault(4, card.initialDisplacement, 0.0);
    writer.realUnlessDefault(5, card.initialVelocity, 0.0);
}

void TransientInitialConditionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A modal coordinate (C = -1) is a mode number, not a point.
    if (m_components.get(row) != TransientInitialConditionCard::MODAL)
        m_node.set(row, visitor.visit("node", ReferenceTarget::ScalarOrGridPoint, m_node.get(row)));
}

std::uint32_t TransientInitialConditionTable::append(const TransientInitialConditionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_node.push(card.node);
    m_components.push(card.components);
    m_initialDisplacement.push(card.initialDisplacement);
    m_initialVelocity.push(card.initialVelocity);
    return row;
}

TransientInitialConditionCard TransientInitialConditionTable::card(std::uint32_t row) const
{
    return TransientInitialConditionCard{m_id.get(row), m_node.get(row), m_components.get(row),
                                         m_initialDisplacement.get(row), m_initialVelocity.get(row)};
}

void TransientInitialConditionTable::store(std::uint32_t row, const TransientInitialConditionCard& card)
{
    m_id.set(row, card.id);
    m_node.set(row, card.node);
    m_components.set(row, card.components);
    m_initialDisplacement.set(row, card.initialDisplacement);
    m_initialVelocity.set(row, card.initialVelocity);
}

void TransientInitialConditionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_components.keepRows(rows);
    m_initialDisplacement.keepRows(rows);
    m_initialVelocity.keepRows(rows);
}

} // namespace oor::model
