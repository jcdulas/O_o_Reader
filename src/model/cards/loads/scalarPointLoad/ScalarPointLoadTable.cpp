#include "oor/model/cards/loads/scalarPointLoad/ScalarPointLoadTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_LOAD_FIELD = 2;

} // namespace

std::optional<ScalarPointLoadCard> ScalarPointLoadFormat::read(FieldReader& reader)
{
    ScalarPointLoadCard card;
    card.id = reader.requiredInteger(1, "id");
    // The pairs (Si, Fi) follow each other from field 2. A pair with both fields blank is padding.
    const int last = reader.lastUsedField();
    for (int field = FIRST_LOAD_FIELD; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        PointLoad load;
        load.node = reader.requiredInteger(field, "scalarPoint");
        load.magnitude = reader.requiredReal(field + 1, "magnitude");
        card.loads.push_back(load);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ScalarPointLoadFormat::check(const ScalarPointLoadCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the load set ID must be a positive integer", "id"};
    if (card.loads.empty())
        return CardProblem{"card.missing-field", "the card needs at least one load", "scalarPoint1"};
    // up to three loads on a card.
    if (card.loads.size() > ScalarPointLoadCard::MAX_LOADS)
        return CardProblem{"card.bad-value", "the card defines at most three loads", "scalarPoint3"};
    for (const PointLoad& load : card.loads)
    {
        if (load.node <= 0)
            return CardProblem{"card.bad-id", "a scalar or grid point ID must be a positive integer", "scalarPoint1"};
    }
    return std::nullopt;
}

void ScalarPointLoadFormat::write(const ScalarPointLoadCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    int field = FIRST_LOAD_FIELD;
    for (const PointLoad& load : card.loads)
    {
        writer.integer(field, load.node);
        writer.real(field + 1, load.magnitude);
        field += 2;
    }
}

std::uint32_t ScalarPointLoadTable::append(const ScalarPointLoadCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    std::vector<base::EntityId> nodes;
    std::vector<double> magnitudes;
    for (const PointLoad& load : card.loads)
    {
        nodes.push_back(load.node);
        magnitudes.push_back(load.magnitude);
    }
    m_node.push(nodes);
    m_magnitude.push(magnitudes);
    return row;
}

ScalarPointLoadCard ScalarPointLoadTable::card(std::uint32_t row) const
{
    ScalarPointLoadCard card;
    card.id = m_id.get(row);
    const auto nodes = m_node.get(row);
    const auto magnitudes = m_magnitude.get(row);
    for (std::size_t index = 0; index < nodes.size(); ++index)
        card.loads.push_back(PointLoad{nodes[index], magnitudes[index]});
    return card;
}

void ScalarPointLoadTable::store(std::uint32_t row, const ScalarPointLoadCard& card)
{
    m_id.set(row, card.id);
    std::vector<base::EntityId> nodes;
    std::vector<double> magnitudes;
    for (const PointLoad& load : card.loads)
    {
        nodes.push_back(load.node);
        magnitudes.push_back(load.magnitude);
    }
    m_node.set(row, nodes);
    m_magnitude.set(row, magnitudes);
}

void ScalarPointLoadTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_node.keepRows(rows);
    m_magnitude.keepRows(rows);
}

void ScalarPointLoadTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // A scalar point may be named without an SPOINT that defines it (a scalar element does), so the check lets it be.
    std::vector<base::EntityId> nodes(m_node.get(row).begin(), m_node.get(row).end());
    bool changed = false;
    for (base::EntityId& node : nodes)
    {
        const base::EntityId updated = visitor.visit("scalarPoint", ReferenceTarget::ScalarOrGridPoint, node);
        changed = changed || updated != node;
        node = updated;
    }
    if (changed)
        m_node.set(row, nodes);
}

} // namespace oor::model
