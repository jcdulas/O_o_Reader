#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr int FIELDS_PER_LINE = 8;
constexpr int FIELDS_PER_TERM = 3;

// The field of the G of term `index` (C and A follow it): two terms on each line of eight fields, from the second
// field of the line on.
int firstFieldOf(std::size_t index)
{
    const int line = static_cast<int>(index / 2);
    const int inLine = static_cast<int>(index % 2);
    return line * FIELDS_PER_LINE + 2 + inLine * FIELDS_PER_TERM;
}

} // namespace

std::optional<MultiPointConstrCard> MultiPointConstrFormat::read(FieldReader& reader)
{
    MultiPointConstrCard card;
    card.id = reader.requiredInteger(1, "id");
    const int last = reader.lastUsedField();
    for (std::size_t index = 0; firstFieldOf(index) <= last && !reader.failed(); ++index)
    {
        const int field = firstFieldOf(index);
        if (reader.isBlank(field) && reader.isBlank(field + 1) && reader.isBlank(field + 2))
            continue; // padding
        MultiPointConstrTerm term;
        term.node = reader.requiredInteger(field, "node");
        term.component = reader.integer(field + 1, "components", 0);
        // format reference: Aj defaults to 0.0, except A1, which must be given and not be zero.
        term.coefficient = card.terms.empty() ? reader.requiredReal(field + 2, "coefficient")
                                              : reader.real(field + 2, "coefficient", 0.0);
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MultiPointConstrFormat::check(const MultiPointConstrCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the constraint set ID must be a positive integer", "id"};
    if (card.terms.empty())
        return CardProblem{"card.missing-field", "the card needs at least one term", "node1"};
    for (const MultiPointConstrTerm& term : card.terms)
    {
        if (term.node <= 0)
            return CardProblem{"card.bad-id", "a grid or scalar point ID must be a positive integer", "node1"};
        if (term.component < 0 || term.component > 6)
            return CardProblem{"card.bad-value", "C must be one of the integers 1 to 6, or 0 for a scalar point",
                               "components1"};
    }
    if (card.terms.front().coefficient == 0.0)
        return CardProblem{
            "card.bad-value",
            "the coefficient of the dependent term (" + std::string(CardProblem::FIELD) + ") must not be zero",
            "coefficient1"};
    return std::nullopt;
}

void MultiPointConstrFormat::write(const MultiPointConstrCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    std::size_t index = 0;
    for (const MultiPointConstrTerm& term : card.terms)
    {
        const int field = firstFieldOf(index++);
        writer.integer(field, term.node);
        writer.integerUnlessDefault(field + 1, term.component, 0);
        writer.real(field + 2, term.coefficient);
    }
}

std::uint32_t MultiPointConstrTable::append(const MultiPointConstrCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrTerm& term : card.terms)
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

MultiPointConstrCard MultiPointConstrTable::card(std::uint32_t row) const
{
    MultiPointConstrCard card;
    card.id = m_id.get(row);
    const auto nodes = m_node.get(row);
    const auto components = m_component.get(row);
    const auto coefficients = m_coefficient.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.terms.push_back(MultiPointConstrTerm{nodes[index], components[index], coefficients[index]});
    return card;
}

void MultiPointConstrTable::store(std::uint32_t row, const MultiPointConstrCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;
    std::vector<double> coefficients;
    for (const MultiPointConstrTerm& term : card.terms)
    {
        nodes.push_back(term.node);
        components.push_back(term.component);
        coefficients.push_back(term.coefficient);
    }
    m_node.set(row, nodes);
    m_component.set(row, components);
    m_coefficient.set(row, coefficients);
}

void MultiPointConstrTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_component.keepRows(rows);
    m_coefficient.keepRows(rows);
}

void MultiPointConstrTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    std::vector<base::EntityId> nodes(m_node.get(row).begin(), m_node.get(row).end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        const base::EntityId updated = visitor.visit("node", ReferenceTarget::Node, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_node.set(row, nodes);
}

} // namespace oor::model
