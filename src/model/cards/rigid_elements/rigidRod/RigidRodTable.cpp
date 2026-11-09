#include "oor/model/cards/rigid_elements/rigidRod/RigidRodTable.hpp"

namespace oor::model
{

std::optional<RigidRodCard> RigidRodFormat::read(FieldReader& reader)
{
    RigidRodCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.componentsAtA = reader.integer(4, "componentsAtA", 0);
    card.componentsAtB = reader.integer(5, "componentsAtB", 0);
    card.alphaCoefficient = reader.real(6, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(7, "referenceTemperature", 0.0);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RigidRodFormat::check(const RigidRodCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    if (card.nodeA <= 0)
        return CardProblem{"card.bad-id",
                           "the grid point " + std::string(CardProblem::FIELD) + " must be a positive integer",
                           "nodeA"};
    if (card.nodeB <= 0 || card.nodeB == card.nodeA)
        return CardProblem{"card.bad-id",
                           std::string(CardProblem::FIELD) +
                               " must be a positive integer, not the same grid point as " +
                               CardProblem::fieldNamed("nodeA"),
                           "nodeB"};
    if (card.componentsAtA < 0 || card.componentsAtA > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or blank",
                           "componentsAtA"};
    if (card.componentsAtB < 0 || card.componentsAtB > 3)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or blank",
                           "componentsAtB"};
    if (card.componentsAtA != 0 && card.componentsAtB != 0)
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("componentsAtA") + " and " + std::string(CardProblem::FIELD) +
                               " cannot both be given: one dependent component only",
                           "componentsAtB"};
    return std::nullopt;
}

void RigidRodFormat::write(const RigidRodCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    writer.integerUnlessDefault(4, card.componentsAtA, 0);
    writer.integerUnlessDefault(5, card.componentsAtB, 0);
    writer.realUnlessDefault(6, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(7, card.referenceTemperature, 0.0);
}

std::uint32_t RigidRodTable::append(const RigidRodCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_componentsAtA.push(card.componentsAtA);
    m_componentsAtB.push(card.componentsAtB);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidRodCard RigidRodTable::card(std::uint32_t row) const
{
    return RigidRodCard{m_id.get(row),
                        m_nodeA.get(row),
                        m_nodeB.get(row),
                        m_componentsAtA.get(row),
                        m_componentsAtB.get(row),
                        m_alphaCoefficient.get(row),
                        m_referenceTemperature.get(row)};
}

void RigidRodTable::store(std::uint32_t row, const RigidRodCard& card)
{
    m_id.set(row, card.id);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_componentsAtA.set(row, card.componentsAtA);
    m_componentsAtB.set(row, card.componentsAtB);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidRodTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_componentsAtA.keepRows(rows);
    m_componentsAtB.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidRodTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
}

} // namespace oor::model
