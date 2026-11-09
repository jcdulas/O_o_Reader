#include "oor/model/cards/dynamics/nonlinearTableLoad1/NonlinearTableLoad1Table.hpp"

namespace oor::model
{

namespace
{

// CI: a component of the point the load is applied to; CJ: a displacement component (0 to 6) or a velocity (10 to 16).
bool validLoadComponent(std::int32_t component)
{
    return component >= 0 && component <= 6;
}

bool validVariableComponent(std::int32_t component)
{
    return (component >= 0 && component <= 6) || (component >= 10 && component <= 16);
}

} // namespace

std::optional<NonlinearTableLoad1Card> NonlinearTableLoad1Format::read(FieldReader& reader)
{
    NonlinearTableLoad1Card card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeI = reader.requiredInteger(2, "nodeI");
    card.componentsI = reader.integer(3, "componentsI", 0);
    card.scaleFactor = reader.requiredReal(4, "scaleFactor");
    card.nodeJ = reader.requiredInteger(5, "nodeJ");
    card.componentsJ = reader.integer(6, "componentsJ", 0);
    card.tableId = reader.requiredInteger(7, "tableId");
    if (reader.lastUsedField() > 7)
        reader.error("card.bad-value", "the card has no field beyond " + std::string("tableId"), "tableId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearTableLoad1Format::check(const NonlinearTableLoad1Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the nonlinear load set ID must be a positive integer", "id"};
    if (card.nodeI <= 0)
        return CardProblem{"card.bad-id", "the point ID must be a positive integer", "nodeI"};
    if (!validLoadComponent(card.componentsI))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 to 6", "componentsI"};
    if (card.nodeJ <= 0)
        return CardProblem{"card.bad-id", "the point ID must be a positive integer", "nodeJ"};
    if (!validVariableComponent(card.componentsJ))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be 0 to 6 (displacement) or 10 to 16 (velocity)",
                           "componentsJ"};
    if (card.tableId <= 0)
        return CardProblem{"card.bad-id", "the table ID must be a positive integer", "tableId"};
    return std::nullopt;
}

void NonlinearTableLoad1Format::write(const NonlinearTableLoad1Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodeI);
    writer.integerUnlessDefault(3, card.componentsI, 0);
    writer.real(4, card.scaleFactor);
    writer.integer(5, card.nodeJ);
    writer.integerUnlessDefault(6, card.componentsJ, 0);
    writer.integer(7, card.tableId);
}

std::uint32_t NonlinearTableLoad1Table::append(const NonlinearTableLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodeI.push(card.nodeI);
    m_componentsI.push(card.componentsI);
    m_scaleFactor.push(card.scaleFactor);
    m_nodeJ.push(card.nodeJ);
    m_componentsJ.push(card.componentsJ);
    m_tableId.push(card.tableId);
    return row;
}

NonlinearTableLoad1Card NonlinearTableLoad1Table::card(std::uint32_t row) const
{
    return NonlinearTableLoad1Card{m_id.get(row),          m_nodeI.get(row), m_componentsI.get(row),
                                   m_scaleFactor.get(row), m_nodeJ.get(row), m_componentsJ.get(row),
                                   m_tableId.get(row)};
}

void NonlinearTableLoad1Table::store(std::uint32_t row, const NonlinearTableLoad1Card& card)
{
    m_id.set(row, card.id);
    m_nodeI.set(row, card.nodeI);
    m_componentsI.set(row, card.componentsI);
    m_scaleFactor.set(row, card.scaleFactor);
    m_nodeJ.set(row, card.nodeJ);
    m_componentsJ.set(row, card.componentsJ);
    m_tableId.set(row, card.tableId);
}

void NonlinearTableLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodeI.keepRows(rows);
    m_componentsI.keepRows(rows);
    m_scaleFactor.keepRows(rows);
    m_nodeJ.keepRows(rows);
    m_componentsJ.keepRows(rows);
    m_tableId.keepRows(rows);
}

void NonlinearTableLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The points are grid, scalar or extra points (extra points are in the node ID space); a scalar point may be named
    // without an SPOINT that defines it.
    m_nodeI.set(row, visitor.visit("nodeI", ReferenceTarget::ScalarOrGridPoint, m_nodeI.get(row)));
    m_nodeJ.set(row, visitor.visit("nodeJ", ReferenceTarget::ScalarOrGridPoint, m_nodeJ.get(row)));
    m_tableId.set(row, visitor.visit("tableId", ReferenceTarget::DynamicTable, m_tableId.get(row)));
}

} // namespace oor::model
