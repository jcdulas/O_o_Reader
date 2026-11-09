#include "oor/model/cards/constraints/multiPointConstrWithValue/MultiPointConstrWithValueTable.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIELDS_PER_TERM = 3;
// SID, GM, CM, AM and YM: the fields of the first line that hold a value.
constexpr int LAST_DEPENDENT_FIELD = 5;

// The field of the G of independent term `index` (C and A follow it): two terms on each continuation line of eight
// fields, from the second field of the line on (the dependent term is alone on the first line).
int firstFieldOf(std::size_t index)
{
    const int line = static_cast<int>(index / 2) + 1;
    const int inLine = static_cast<int>(index % 2);
    return line * FIELDS_PER_LINE + 2 + inLine * FIELDS_PER_TERM;
}

// True when the Card layout has a value in `field`: the five fields of the first line and the six fields of a
// continuation line that the two terms take (the first and the last field of a continuation line are blank).
bool holdsValue(int field)
{
    if (field <= LAST_DEPENDENT_FIELD)
        return true;
    const int inLine = (field - 1) % FIELDS_PER_LINE;
    return field > FIELDS_PER_LINE && inLine >= 1 && inLine <= 2 * FIELDS_PER_TERM;
}

bool isSingleComponent(std::int32_t component) noexcept
{
    return component >= 0 && component <= 6;
}

} // namespace

std::optional<MultiPointConstrWithValueCard> MultiPointConstrWithValueFormat::read(FieldReader& reader)
{
    MultiPointConstrWithValueCard card;
    card.id = reader.requiredInteger(1, "id");
    card.node = reader.requiredInteger(2, "dependentNode");
    card.component = reader.integer(3, "dependentComponents", 0);
    card.coefficient = reader.requiredReal(4, "dependentCoefficient");
    card.value = reader.real(5, "rightHandSide", 0.0);
    const int last = reader.lastUsedField();
    // A value in a field the layout leaves blank would be dropped: refuse it instead.
    for (int field = LAST_DEPENDENT_FIELD + 1; field <= last; ++field)
    {
        if (!holdsValue(field) && !reader.isBlank(field))
        {
            reader.error("card.bad-value", "field " + std::to_string(field + 1) + " of an " +
                                               std::string(reader.cardName()) + " is not used");
            break;
        }
    }
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        MultiPointConstrWithValueTerm term;
        term.node = reader.requiredInteger(field, "nodes");
        term.component = reader.integer(field + 1, "componentList", 0);
        term.coefficient = reader.real(field + 2, "coefficients", 0.0);
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MultiPointConstrWithValueFormat::check(const MultiPointConstrWithValueCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.node <= 0)
        return CardProblem{"card.bad-id", "the grid or scalar point ID must be a positive integer", "dependentNode"};
    if (!isSingleComponent(card.component))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar point",
            "dependentComponents"};
    if (!std::isfinite(card.coefficient) || card.coefficient == 0.0)
        return CardProblem{
            "card.bad-value",
            "the coefficient of the dependent term (" + std::string(CardProblem::FIELD) + ") must not be zero",
            "dependentCoefficient"};
    if (!std::isfinite(card.value))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "rightHandSide"};
    for (const MultiPointConstrWithValueTerm& term : card.terms)
    {
        if (term.node <= 0)
            return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer", "nodes"};
        if (!isSingleComponent(term.component))
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be one of the integers 1 to 6, or 0 for a scalar point",
                "componentList"};
        if (!std::isfinite(term.coefficient))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                               "coefficients"};
    }
    return std::nullopt;
}

void MultiPointConstrWithValueFormat::write(const MultiPointConstrWithValueCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.node);
    writer.integerUnlessDefault(3, card.component, 0);
    writer.real(4, card.coefficient);
    writer.realUnlessDefault(5, card.value, 0.0);
    std::size_t index = 0;
    for (const MultiPointConstrWithValueTerm& term : card.terms)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, term.node);
        writer.integerUnlessDefault(field + 1, term.component, 0);
        // A coefficient of 0.0 is the format reference default, but a term that is all blank would read back as
        // padding.
        writer.real(field + 2, term.coefficient);
    }
}

std::uint32_t MultiPointConstrWithValueTable::append(const MultiPointConstrWithValueCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_dependentNode.push(card.node);
    m_dependentComponents.push(card.component);
    m_dependentCoefficient.push(card.coefficient);
    m_rightHandSide.push(card.value);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrWithValueTerm& term : card.terms)
    {
        nodes.push_back(term.node);
        components.push_back(term.component);
        coefficients.push_back(term.coefficient);
    }
    m_node.push(nodes);
    m_component.push(components);
    m_coefficient.push(coefficients);
    return row;
}

MultiPointConstrWithValueCard MultiPointConstrWithValueTable::card(std::uint32_t row) const
{
    MultiPointConstrWithValueCard card;
    card.id = m_id.get(row);
    card.node = m_dependentNode.get(row);
    card.component = m_dependentComponents.get(row);
    card.coefficient = m_dependentCoefficient.get(row);
    card.value = m_rightHandSide.get(row);
    const auto nodes = m_node.get(row);
    const auto components = m_component.get(row);
    const auto coefficients = m_coefficient.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.terms.push_back(MultiPointConstrWithValueTerm{nodes[index], components[index], coefficients[index]});
    return card;
}

void MultiPointConstrWithValueTable::store(std::uint32_t row, const MultiPointConstrWithValueCard& card)
{
    m_id.set(row, card.id);
    m_dependentNode.set(row, card.node);
    m_dependentComponents.set(row, card.component);
    m_dependentCoefficient.set(row, card.coefficient);
    m_rightHandSide.set(row, card.value);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrWithValueTerm& term : card.terms)
    {
        nodes.push_back(term.node);
        components.push_back(term.component);
        coefficients.push_back(term.coefficient);
    }
    m_node.set(row, nodes);
    m_component.set(row, components);
    m_coefficient.set(row, coefficients);
}

void MultiPointConstrWithValueTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_dependentNode.keepRows(rows);
    m_dependentComponents.keepRows(rows);
    m_dependentCoefficient.keepRows(rows);
    m_rightHandSide.keepRows(rows);
    m_node.keepRows(rows);
    m_component.keepRows(rows);
    m_coefficient.keepRows(rows);
}

void MultiPointConstrWithValueTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_dependentNode.set(row,
                        visitor.visit("dependentNode", ReferenceTarget::ScalarOrGridPoint, m_dependentNode.get(row)));
    std::vector<base::EntityId> nodes(m_node.get(row).begin(), m_node.get(row).end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        const base::EntityId updated = visitor.visit("nodes", ReferenceTarget::ScalarOrGridPoint, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_node.set(row, nodes);
}

} // namespace oor::model
