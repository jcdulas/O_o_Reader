#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"

namespace oor::model
{

namespace
{

// The fields: EID GA GB GC CNA CNB CNC on the first line (field 8 is blank), CMA CMB CMC ALPHA TREF on the next.
constexpr int FIRST_NODE_FIELD = 2;
constexpr int FIRST_INDEPENDENT_FIELD = 5;
constexpr int FIRST_DEPENDENT_FIELD = 9;
constexpr int ALPHA_FIELD = 12;
constexpr int REFERENCE_TEMPERATURE_FIELD = 13;

} // namespace

void RigidTriaPlateFormat::resolveDefaults(RigidTriaPlateCard& card) noexcept
{
    if (card.independent[0] == 0 && card.independent[1] == 0 && card.independent[2] == 0)
        card.independent[0] = components::ALL;
}

std::optional<RigidTriaPlateCard> RigidTriaPlateFormat::read(FieldReader& reader)
{
    static constexpr const char* NODE_NAMES[] = {"nodeA", "nodeB", "nodeC"};
    static constexpr const char* INDEPENDENT_NAMES[] = {"independentComponentsA", "independentComponentsB",
                                                        "independentComponentsC"};
    static constexpr const char* DEPENDENT_NAMES[] = {"componentsAtA", "componentsAtB", "componentsAtC"};
    RigidTriaPlateCard card;
    card.id = reader.requiredInteger(1, "id");
    for (int index = 0; index < 3; ++index)
    {
        card.nodes[static_cast<std::size_t>(index)] =
            reader.requiredInteger(FIRST_NODE_FIELD + index, NODE_NAMES[index]);
        card.independent[static_cast<std::size_t>(index)] =
            base::sortedDigits(reader.integer(FIRST_INDEPENDENT_FIELD + index, INDEPENDENT_NAMES[index], 0));
        card.dependent[static_cast<std::size_t>(index)] =
            base::sortedDigits(reader.integer(FIRST_DEPENDENT_FIELD + index, DEPENDENT_NAMES[index], 0));
    }
    card.alphaCoefficient = reader.real(ALPHA_FIELD, "alphaCoefficient", 0.0);
    card.referenceTemperature = reader.real(REFERENCE_TEMPERATURE_FIELD, "referenceTemperature", 0.0);
    if (!reader.failed())
    {
        resolveDefaults(card);
        reader.report(check(card));
    }
    return card;
}

std::optional<CardProblem> RigidTriaPlateFormat::check(const RigidTriaPlateCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the element ID must be a positive integer", "id"};
    static constexpr const char* NODE_NAMES[] = {"nodeA", "nodeB", "nodeC"};
    static constexpr const char* INDEPENDENT_NAMES[] = {"independentComponentsA", "independentComponentsB",
                                                        "independentComponentsC"};
    static constexpr const char* DEPENDENT_NAMES[] = {"componentsAtA", "componentsAtB", "componentsAtC"};
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (card.nodes[index] <= 0)
            return CardProblem{"card.bad-id", "a grid point ID must be a positive integer", NODE_NAMES[index]};
        for (std::size_t other = 0; other < index; ++other)
        {
            if (card.nodes[other] == card.nodes[index])
                return CardProblem{"card.bad-id", "the three grid points must be different", NODE_NAMES[index]};
        }
        if (!base::isComponentSet(card.independent[index]))
            return CardProblem{"card.bad-value",
                               "the components must be 0 or a list of the digits 1 to 6, each at most once",
                               INDEPENDENT_NAMES[index]};
        if (!base::isComponentSet(card.dependent[index]))
            return CardProblem{"card.bad-value",
                               "the components must be 0 or a list of the digits 1 to 6, each at most once",
                               DEPENDENT_NAMES[index]};
        if (components::overlap(card.independent[index], card.dependent[index]))
            return CardProblem{"card.bad-value", "a component cannot be both independent and dependent at a grid point",
                               DEPENDENT_NAMES[index]};
    }
    if (card.independent[0] == 0 && card.independent[1] == 0 && card.independent[2] == 0)
        return CardProblem{"card.missing-field", "the plate has no independent component", "independentComponentsA"};
    return std::nullopt;
}

void RigidTriaPlateFormat::write(const RigidTriaPlateCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (int index = 0; index < 3; ++index)
    {
        const auto position = static_cast<std::size_t>(index);
        writer.integer(FIRST_NODE_FIELD + index, card.nodes[position]);
        // CNA depends on CNB and CNC when they are blank: a blank written here is a zero, never the default.
        if (card.independent[position] != 0)
            writer.integer(FIRST_INDEPENDENT_FIELD + index, card.independent[position]);
        writer.integerUnlessDefault(FIRST_DEPENDENT_FIELD + index, card.dependent[position], 0);
    }
    writer.realUnlessDefault(ALPHA_FIELD, card.alphaCoefficient, 0.0);
    writer.realUnlessDefault(REFERENCE_TEMPERATURE_FIELD, card.referenceTemperature, 0.0);
}

std::uint32_t RigidTriaPlateTable::append(const RigidTriaPlateCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodes.push(card.nodes);
    m_independentComponents.push(card.independent);
    m_dependentComponents.push(card.dependent);
    m_alphaCoefficient.push(card.alphaCoefficient);
    m_referenceTemperature.push(card.referenceTemperature);
    return row;
}

RigidTriaPlateCard RigidTriaPlateTable::card(std::uint32_t row) const
{
    return RigidTriaPlateCard{m_id.get(row),
                              m_nodes.get(row),
                              m_independentComponents.get(row),
                              m_dependentComponents.get(row),
                              m_alphaCoefficient.get(row),
                              m_referenceTemperature.get(row)};
}

void RigidTriaPlateTable::store(std::uint32_t row, const RigidTriaPlateCard& card)
{
    m_id.set(row, card.id);
    m_nodes.set(row, card.nodes);
    m_independentComponents.set(row, card.independent);
    m_dependentComponents.set(row, card.dependent);
    m_alphaCoefficient.set(row, card.alphaCoefficient);
    m_referenceTemperature.set(row, card.referenceTemperature);
}

void RigidTriaPlateTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodes.keepRows(rows);
    m_independentComponents.keepRows(rows);
    m_dependentComponents.keepRows(rows);
    m_alphaCoefficient.keepRows(rows);
    m_referenceTemperature.keepRows(rows);
}

void RigidTriaPlateTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::array<base::EntityId, 3> nodes = m_nodes.get(row);
    nodes[0] = visitor.visit("nodeA", ReferenceTarget::Node, nodes[0]);
    nodes[1] = visitor.visit("nodeB", ReferenceTarget::Node, nodes[1]);
    nodes[2] = visitor.visit("nodeC", ReferenceTarget::Node, nodes[2]);
    m_nodes.set(row, nodes);
}

} // namespace oor::model
