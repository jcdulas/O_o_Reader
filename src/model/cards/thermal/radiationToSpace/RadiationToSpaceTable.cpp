#include "oor/model/cards/thermal/radiationToSpace/RadiationToSpaceTable.hpp"

#include <string>

namespace oor::model
{

std::optional<RadiationToSpaceCard> RadiationToSpaceFormat::read(FieldReader& reader)
{
    RadiationToSpaceCard card;
    card.ambientNode = reader.requiredInteger(1, "ambientNode");
    card.ambientViewFactor = reader.requiredReal(2, "ambientViewFactor");
    card.controlNode = reader.integer(3, "controlNode", 0);
    card.elements = readThermalRuns(reader, 4, "elements", true);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationToSpaceFormat::check(const RadiationToSpaceCard& card)
{
    if (card.ambientNode <= 0)
        return CardProblem{
            "card.bad-id",
            std::string("the ambient point ") + std::string(CardProblem::FIELD) + " must be a positive integer",
            "ambientNode"};
    if (!(card.ambientViewFactor > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "ambientViewFactor"};
    if (card.controlNode < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " cannot be negative", "controlNode"};
    return checkThermalRuns(card.elements, "elements", false);
}

void RadiationToSpaceFormat::write(const RadiationToSpaceCard& card, FieldWriter& writer)
{
    writer.integer(1, card.ambientNode);
    writer.real(2, card.ambientViewFactor);
    writer.integerUnlessDefault(3, card.controlNode, 0);
    writeThermalRuns(writer, 4, card.elements);
}

std::uint32_t RadiationToSpaceTable::append(const RadiationToSpaceCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_ambientNode.push(card.ambientNode);
    m_ambientViewFactor.push(card.ambientViewFactor);
    m_controlNode.push(card.controlNode);
    m_runs.push(toThermalTriples(card.elements));
    return row;
}

RadiationToSpaceCard RadiationToSpaceTable::card(std::uint32_t row) const
{
    RadiationToSpaceCard card;
    card.ambientNode = m_ambientNode.get(row);
    card.ambientViewFactor = m_ambientViewFactor.get(row);
    card.controlNode = m_controlNode.get(row);
    card.elements = toThermalRuns(m_runs.get(row));
    return card;
}

void RadiationToSpaceTable::store(std::uint32_t row, const RadiationToSpaceCard& card)
{
    m_ambientNode.set(row, card.ambientNode);
    m_ambientViewFactor.set(row, card.ambientViewFactor);
    m_controlNode.set(row, card.controlNode);
    m_runs.set(row, toThermalTriples(card.elements));
}

void RadiationToSpaceTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_ambientNode.keepRows(rows);
    m_ambientViewFactor.keepRows(rows);
    m_controlNode.keepRows(rows);
    m_runs.keepRows(rows);
}

void RadiationToSpaceTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_ambientNode.set(row, visitor.visit("ambientNode", ReferenceTarget::ScalarOrGridPoint, m_ambientNode.get(row)));
    if (const base::EntityId controlNode = m_controlNode.get(row); controlNode > 0)
        m_controlNode.set(row, visitor.visit("controlNode", ReferenceTarget::ScalarOrGridPoint, controlNode));
    visitThermalRuns(m_runs, row, visitor, "elements", ReferenceTarget::SurfaceElement,
                     ReferenceTarget::SurfaceElementInRange);
}

} // namespace oor::model
