#include "oor/model/cards/matrices/monitorIntegratedLoad1/MonitorIntegratedLoad1Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"

#include <cmath>

namespace oor::model
{

std::optional<MonitorIntegratedLoad1Card> MonitorIntegratedLoad1Format::read(FieldReader& reader)
{
    MonitorIntegratedLoad1Card card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.outputAxes = base::sortedDigits(reader.requiredInteger(9, "outputAxes"));
    card.component = std::string(reader.text(10));
    card.positionFrameId = reader.integer(11, "positionFrameId", 0);
    card.xCoord = reader.real(12, "xCoord", 0.0);
    card.yCoord = reader.real(13, "yCoord", 0.0);
    card.zCoord = reader.real(14, "zCoord", 0.0);
    card.displacementFrameId =
        reader.integer(15, "displacementFrameId", card.positionFrameId); // the format reference: the system of CP
    if (reader.lastUsedField() > 15)
        reader.error(
            "card.bad-value",
            "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("displacementFrameId"),
            "displacementFrameId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorIntegratedLoad1Format::check(const MonitorIntegratedLoad1Card& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (card.outputAxes <= 0 || !base::isComponentSet(card.outputAxes))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a set of the digits 1 to 6",
                           "outputAxes"};
    if (card.component.empty())
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " must name an aerodynamic component", "component"};
    if (card.positionFrameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "positionFrameId"};
    if (card.displacementFrameId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative",
                           "displacementFrameId"};
    if (!std::isfinite(card.xCoord) || !std::isfinite(card.yCoord) || !std::isfinite(card.zCoord))
        return CardProblem{"card.bad-value",
                           "{field}, " + CardProblem::fieldNamed("yCoord") + " and " +
                               CardProblem::fieldNamed("zCoord") + " must be finite numbers",
                           "xCoord"};
    return std::nullopt;
}

void MonitorIntegratedLoad1Format::write(const MonitorIntegratedLoad1Card& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writer.integer(9, card.outputAxes);
    writer.text(10, card.component);
    writer.integerUnlessDefault(11, card.positionFrameId, 0);
    writer.realUnlessDefault(12, card.xCoord, 0.0);
    writer.realUnlessDefault(13, card.yCoord, 0.0);
    writer.realUnlessDefault(14, card.zCoord, 0.0);
    writer.integer(15, card.displacementFrameId); // resolved: its default is CP
}

std::uint32_t MonitorIntegratedLoad1Table::append(const MonitorIntegratedLoad1Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_label.push(card.label);
    m_outputAxes.push(card.outputAxes);
    m_component.push(card.component);
    m_positionFrameId.push(card.positionFrameId);
    m_xCoord.push(card.xCoord);
    m_yCoord.push(card.yCoord);
    m_zCoord.push(card.zCoord);
    m_displacementFrameId.push(card.displacementFrameId);
    return row;
}

MonitorIntegratedLoad1Card MonitorIntegratedLoad1Table::card(std::uint32_t row) const
{
    MonitorIntegratedLoad1Card card;
    card.name = m_name.get(row);
    card.label = m_label.get(row);
    card.outputAxes = m_outputAxes.get(row);
    card.component = m_component.get(row);
    card.positionFrameId = m_positionFrameId.get(row);
    card.xCoord = m_xCoord.get(row);
    card.yCoord = m_yCoord.get(row);
    card.zCoord = m_zCoord.get(row);
    card.displacementFrameId = m_displacementFrameId.get(row);
    return card;
}

void MonitorIntegratedLoad1Table::store(std::uint32_t row, const MonitorIntegratedLoad1Card& card)
{
    m_name.set(row, card.name);
    m_label.set(row, card.label);
    m_outputAxes.set(row, card.outputAxes);
    m_component.set(row, card.component);
    m_positionFrameId.set(row, card.positionFrameId);
    m_xCoord.set(row, card.xCoord);
    m_yCoord.set(row, card.yCoord);
    m_zCoord.set(row, card.zCoord);
    m_displacementFrameId.set(row, card.displacementFrameId);
}

void MonitorIntegratedLoad1Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_label.keepRows(rows);
    m_outputAxes.keepRows(rows);
    m_component.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_xCoord.keepRows(rows);
    m_yCoord.keepRows(rows);
    m_zCoord.keepRows(rows);
    m_displacementFrameId.keepRows(rows);
}

void MonitorIntegratedLoad1Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The system of the coordinates and the system of the output; the basic system (0) is not a reference.
    if (const base::EntityId cp = m_positionFrameId.get(row); cp > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, cp));
    if (const base::EntityId displacementFrame = m_displacementFrameId.get(row); displacementFrame > 0)
        m_displacementFrameId.set(row,
                                  visitor.visit("displacementFrameId", ReferenceTarget::Coordinate, displacementFrame));
}

} // namespace oor::model
