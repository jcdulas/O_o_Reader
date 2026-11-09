#include "oor/model/cards/dynamics/transferFunction/TransferFunctionTable.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_FIELD = 9;
constexpr int TERM_STRIDE = 8; // one independent degree of freedom on each line

} // namespace

std::optional<TransferFunctionCard> TransferFunctionFormat::read(FieldReader& reader)
{
    TransferFunctionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeD = reader.requiredInteger(2, "nodeD");
    card.componentsD = reader.integer(3, "componentsD", 0);
    card.coefficientB0 = reader.real(4, "coefficientB0", 0.0);
    card.coefficientB1 = reader.real(5, "coefficientB1", 0.0);
    card.coefficientB2 = reader.real(6, "coefficientB2", 0.0);
    for (int field = 7; field < FIRST_TERM_FIELD; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value", "fields 7 and 8 of the card are not used", "coefficientB2");
    }
    const int last = reader.lastUsedField();
    for (int first = FIRST_TERM_FIELD; first <= last && !reader.failed(); first += TERM_STRIDE)
    {
        TransferFunctionTerm term;
        term.node = reader.requiredInteger(first, "termNodes");
        term.components = reader.integer(first + 1, "termComponents", 0);
        term.coefficientA0 = reader.real(first + 2, "coefficientsA0", 0.0);
        term.coefficientA1 = reader.real(first + 3, "coefficientsA1", 0.0);
        term.coefficientA2 = reader.real(first + 4, "coefficientsA2", 0.0);
        for (int field = first + 5; field < first + TERM_STRIDE; ++field)
        {
            if (!reader.isBlank(field))
                reader.error("card.bad-value",
                             "a line of terms has five fields: a point, its component and three coefficients",
                             "coefficientsA2");
        }
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransferFunctionFormat::check(const TransferFunctionCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the set ID must be a positive integer", "id"};
    if (card.nodeD <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer", "nodeD"};
    if (card.componentsD < 0 || card.componentsD > 6)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be a component from 1 to 6, or 0 for a scalar or extra point",
            "componentsD"};
    if (!std::isfinite(card.coefficientB0) || !std::isfinite(card.coefficientB1) || !std::isfinite(card.coefficientB2))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("coefficientB1") + " and " +
                               CardProblem::fieldNamed("coefficientB2") + " must be finite numbers",
                           "coefficientB0"};
    for (const TransferFunctionTerm& term : card.terms)
    {
        if (term.node <= 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                               "termNodes"};
        if (term.components < 0 || term.components > 6)
            return CardProblem{
                "card.bad-value",
                std::string(CardProblem::FIELD) + " must be a component from 1 to 6, or 0 for a scalar or extra point",
                "termComponents"};
        if (!std::isfinite(term.coefficientA0) || !std::isfinite(term.coefficientA1) ||
            !std::isfinite(term.coefficientA2))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + ", " + CardProblem::fieldNamed("coefficientsA1") +
                                   " and " + CardProblem::fieldNamed("coefficientsA2") + " must be finite numbers",
                               "coefficientsA0"};
    }
    return std::nullopt;
}

void TransferFunctionFormat::write(const TransferFunctionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.nodeD);
    writer.integerUnlessDefault(3, card.componentsD, 0);
    writer.realUnlessDefault(4, card.coefficientB0, 0.0);
    writer.realUnlessDefault(5, card.coefficientB1, 0.0);
    writer.realUnlessDefault(6, card.coefficientB2, 0.0);
    int first = FIRST_TERM_FIELD;
    for (const TransferFunctionTerm& term : card.terms)
    {
        writer.integer(first, term.node);
        writer.integerUnlessDefault(first + 1, term.components, 0);
        writer.realUnlessDefault(first + 2, term.coefficientA0, 0.0);
        writer.realUnlessDefault(first + 3, term.coefficientA1, 0.0);
        writer.realUnlessDefault(first + 4, term.coefficientA2, 0.0);
        first += TERM_STRIDE;
    }
}

void TransferFunctionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_nodeD.set(row, visitor.visit("nodeD", ReferenceTarget::ScalarOrGridPoint, m_nodeD.get(row)));
    const auto points = m_termNodes.get(row);
    std::vector<base::EntityId> updated(points.begin(), points.end());
    bool changed = false;
    for (base::EntityId& point : updated)
    {
        const base::EntityId renumbered = visitor.visit("termNodes", ReferenceTarget::ScalarOrGridPoint, point);
        changed = changed || renumbered != point;
        point = renumbered;
    }
    if (changed)
        m_termNodes.set(row, updated);
}

std::uint32_t TransferFunctionTable::append(const TransferFunctionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_nodeD.push(card.nodeD);
    m_componentsD.push(card.componentsD);
    m_coefficientB0.push(card.coefficientB0);
    m_coefficientB1.push(card.coefficientB1);
    m_coefficientB2.push(card.coefficientB2);
    std::vector<base::EntityId> termNodes;
    std::vector<std::int32_t> termComponents;
    std::vector<double> coefficientsA0, coefficientsA1, coefficientsA2;
    for (const TransferFunctionTerm& term : card.terms)
    {
        termNodes.push_back(term.node);
        termComponents.push_back(term.components);
        coefficientsA0.push_back(term.coefficientA0);
        coefficientsA1.push_back(term.coefficientA1);
        coefficientsA2.push_back(term.coefficientA2);
    }
    m_termNodes.push(termNodes);
    m_termComponents.push(termComponents);
    m_coefficientsA0.push(coefficientsA0);
    m_coefficientsA1.push(coefficientsA1);
    m_coefficientsA2.push(coefficientsA2);
    return row;
}

TransferFunctionCard TransferFunctionTable::card(std::uint32_t row) const
{
    TransferFunctionCard card;
    card.id = m_id.get(row);
    card.nodeD = m_nodeD.get(row);
    card.componentsD = m_componentsD.get(row);
    card.coefficientB0 = m_coefficientB0.get(row);
    card.coefficientB1 = m_coefficientB1.get(row);
    card.coefficientB2 = m_coefficientB2.get(row);
    const auto termNodes = m_termNodes.get(row);
    const auto termComponents = m_termComponents.get(row);
    const auto coefficientsA0 = m_coefficientsA0.get(row);
    const auto coefficientsA1 = m_coefficientsA1.get(row);
    const auto coefficientsA2 = m_coefficientsA2.get(row);
    for (std::size_t index = 0; index < termNodes.size(); ++index)
        card.terms.push_back(TransferFunctionTerm{termNodes[index], termComponents[index], coefficientsA0[index],
                                                  coefficientsA1[index], coefficientsA2[index]});
    return card;
}

void TransferFunctionTable::store(std::uint32_t row, const TransferFunctionCard& card)
{
    m_id.set(row, card.id);
    m_nodeD.set(row, card.nodeD);
    m_componentsD.set(row, card.componentsD);
    m_coefficientB0.set(row, card.coefficientB0);
    m_coefficientB1.set(row, card.coefficientB1);
    m_coefficientB2.set(row, card.coefficientB2);
    std::vector<base::EntityId> termNodes;
    std::vector<std::int32_t> termComponents;
    std::vector<double> coefficientsA0, coefficientsA1, coefficientsA2;
    for (const TransferFunctionTerm& term : card.terms)
    {
        termNodes.push_back(term.node);
        termComponents.push_back(term.components);
        coefficientsA0.push_back(term.coefficientA0);
        coefficientsA1.push_back(term.coefficientA1);
        coefficientsA2.push_back(term.coefficientA2);
    }
    m_termNodes.set(row, termNodes);
    m_termComponents.set(row, termComponents);
    m_coefficientsA0.set(row, coefficientsA0);
    m_coefficientsA1.set(row, coefficientsA1);
    m_coefficientsA2.set(row, coefficientsA2);
}

void TransferFunctionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_nodeD.keepRows(rows);
    m_componentsD.keepRows(rows);
    m_coefficientB0.keepRows(rows);
    m_coefficientB1.keepRows(rows);
    m_coefficientB2.keepRows(rows);
    m_termNodes.keepRows(rows);
    m_termComponents.keepRows(rows);
    m_coefficientsA0.keepRows(rows);
    m_coefficientsA1.keepRows(rows);
    m_coefficientsA2.keepRows(rows);
}

} // namespace oor::model
