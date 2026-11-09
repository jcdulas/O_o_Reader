#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

namespace oor::model
{

void RigidBarFormat::resolveDefaults(RigidBarCard& card) noexcept
{
    // with CNA and CNB both blank, CNA is 123456.
    if (card.independentComponentsA == 0 && card.independentComponentsB == 0)
        card.independentComponentsA = components::ALL;
    // with CMA and CMB both blank, every component that is not independent is dependent.
    if (card.componentsAtA == 0 && card.componentsAtB == 0)
    {
        card.componentsAtA = components::without(components::ALL, card.independentComponentsA);
        card.componentsAtB = components::without(components::ALL, card.independentComponentsB);
    }
}

std::optional<RigidBarCard> RigidBarFormat::read(FieldReader& reader)
{
    RigidBarCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeA = reader.requiredInteger(2, "nodeA");
    card.nodeB = reader.requiredInteger(3, "nodeB");
    card.independentComponentsA = base::sortedDigits(reader.integer(4, "independentComponentsA", 0));
    card.independentComponentsB = base::sortedDigits(reader.integer(5, "independentComponentsB", 0));
    card.componentsAtA = base::sortedDigits(reader.integer(6, "componentsAtA", 0));
    card.componentsAtB = base::sortedDigits(reader.integer(7, "componentsAtB", 0));
    card.alphaCoefficient = reader.real(8, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(9, "referenceTemperature", 0.0);
    if (!reader.failed())
    {
        resolveDefaults(card);
        reader.report(check(card));
    }
    return card;
}

std::optional<CardProblem> RigidBarFormat::check(const RigidBarCard& card)
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
    if (!base::isComponentSet(card.independentComponentsA))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "independentComponentsA"};
    if (!base::isComponentSet(card.independentComponentsB))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "independentComponentsB"};
    if (!base::isComponentSet(card.componentsAtA))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "componentsAtA"};
    if (!base::isComponentSet(card.componentsAtB))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be 0 or a list of the digits 1 to 6, each at most once",
            "componentsAtB"};
    if (card.independentComponentsA == 0 && card.independentComponentsB == 0)
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("independentComponentsB") +
                               " are both empty: the bar has no independent component",
                           "independentComponentsA"};
    if (components::overlap(card.independentComponentsA, card.componentsAtA))
        return CardProblem{
            "card.bad-value",
            "a component cannot be both independent and dependent at " + CardProblem::fieldNamed("nodeA"),
            "componentsAtA"};
    if (components::overlap(card.independentComponentsB, card.componentsAtB))
        return CardProblem{
            "card.bad-value",
            "a component cannot be both independent and dependent at " + CardProblem::fieldNamed("nodeB"),
            "componentsAtB"};
    return std::nullopt;
}

void RigidBarFormat::write(const RigidBarCard& card, FieldWriter& writer)
{
    // CNA, CMA and CMB default to values that depend on other fields: they are written whenever they are not empty, and
    // a blank one is what the reader gives back as the same value.
    writer.integer(1, card.id);
    writer.integer(2, card.nodeA);
    writer.integer(3, card.nodeB);
    if (card.independentComponentsA != 0)
        writer.integer(4, card.independentComponentsA);
    if (card.independentComponentsB != 0)
        writer.integer(5, card.independentComponentsB);
    if (card.componentsAtA != 0)
        writer.integer(6, card.componentsAtA);
    if (card.componentsAtB != 0)
        writer.integer(7, card.componentsAtB);
    writer.realUnlessDefault(8, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(9, card.referenceTemperature, 0.0);
}

std::uint32_t RigidBarTable::append(const RigidBarCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodeA.push(card.nodeA);
    m_nodeB.push(card.nodeB);
    m_independentComponentsA.push(card.independentComponentsA);
    m_independentComponentsB.push(card.independentComponentsB);
    m_componentsAtA.push(card.componentsAtA);
    m_componentsAtB.push(card.componentsAtB);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidBarCard RigidBarTable::card(std::uint32_t row) const
{
    return RigidBarCard{m_id.get(row),
                        m_nodeA.get(row),
                        m_nodeB.get(row),
                        m_independentComponentsA.get(row),
                        m_independentComponentsB.get(row),
                        m_componentsAtA.get(row),
                        m_componentsAtB.get(row),
                        m_alphaCoefficient.get(row),
                        m_referenceTemperature.get(row)};
}

void RigidBarTable::store(std::uint32_t row, const RigidBarCard& card)
{
    m_id.set(row, card.id);
    m_nodeA.set(row, card.nodeA);
    m_nodeB.set(row, card.nodeB);
    m_independentComponentsA.set(row, card.independentComponentsA);
    m_independentComponentsB.set(row, card.independentComponentsB);
    m_componentsAtA.set(row, card.componentsAtA);
    m_componentsAtB.set(row, card.componentsAtB);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidBarTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodeA.keepRows(rows);
    m_nodeB.keepRows(rows);
    m_independentComponentsA.keepRows(rows);
    m_independentComponentsB.keepRows(rows);
    m_componentsAtA.keepRows(rows);
    m_componentsAtB.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidBarTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeA.set(row, visitor.visit("nodeA", ReferenceTarget::Node, m_nodeA.get(row)));
    m_nodeB.set(row, visitor.visit("nodeB", ReferenceTarget::Node, m_nodeB.get(row)));
}

} // namespace oor::model
