#include "oor/model/cards/matrices/monitorNodeForceSum3/MonitorNodeForceSum3Table.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"
#include "oor/model/core/Model.hpp"

#include <cmath>
#include <set>

namespace oor::model
{

namespace
{

constexpr std::string_view EXCLUSION_LETTERS = "SMALPDC";

} // namespace

std::optional<MonitorNodeForceSum3Card> MonitorNodeForceSum3Format::read(FieldReader& reader)
{
    MonitorNodeForceSum3Card card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.outputAxes = base::sortedDigits(reader.requiredInteger(9, "outputAxes"));
    card.nodeSetId = reader.requiredInteger(10, "nodeSetId");
    card.elementSet = reader.integer(11, "elementSetId", 0);
    card.positionFrameId = reader.integer(12, "positionFrameId", 0);
    card.xCoord = reader.real(13, "xCoord", 0.0);
    card.yCoord = reader.real(14, "yCoord", 0.0);
    card.zCoord = reader.real(15, "zCoord", 0.0);
    card.excluded = base::toUpperAscii(reader.text(16));
    card.displacementFrameId =
        reader.integer(17, "displacementFrameId", card.positionFrameId); // the format reference: the system of CP
    if (reader.lastUsedField() > 17)
        reader.error(
            "card.bad-value",
            "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("displacementFrameId"),
            "displacementFrameId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorNodeForceSum3Format::check(const MonitorNodeForceSum3Card& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (card.outputAxes <= 0 || !base::isComponentSet(card.outputAxes))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a set of the digits 1 to 6",
                           "outputAxes"};
    if (card.nodeSetId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a set of grid points",
                           "nodeSetId"};
    if (card.elementSet < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "elementSetId"};
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
    for (const char letter : card.excluded)
    {
        if (EXCLUSION_LETTERS.find(letter) == std::string_view::npos)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is a list of the letters S, M, A, L, P, D and C",
                               "excludedForces"};
    }
    return std::nullopt;
}

void MonitorNodeForceSum3Format::write(const MonitorNodeForceSum3Card& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writer.integer(9, card.outputAxes);
    writer.integer(10, card.nodeSetId);
    writer.integerUnlessDefault(11, card.elementSet, 0);
    writer.integerUnlessDefault(12, card.positionFrameId, 0);
    writer.realUnlessDefault(13, card.xCoord, 0.0);
    writer.realUnlessDefault(14, card.yCoord, 0.0);
    writer.realUnlessDefault(15, card.zCoord, 0.0);
    writer.text(16, card.excluded);
    writer.integer(17, card.displacementFrameId); // resolved: its default is CP
}

std::uint32_t MonitorNodeForceSum3Table::append(const MonitorNodeForceSum3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_label.push(card.label);
    m_outputAxes.push(card.outputAxes);
    m_nodeSetId.push(card.nodeSetId);
    m_elementSet.push(card.elementSet);
    m_positionFrameId.push(card.positionFrameId);
    m_xCoord.push(card.xCoord);
    m_yCoord.push(card.yCoord);
    m_zCoord.push(card.zCoord);
    m_excluded.push(card.excluded);
    m_displacementFrameId.push(card.displacementFrameId);
    return row;
}

MonitorNodeForceSum3Card MonitorNodeForceSum3Table::card(std::uint32_t row) const
{
    MonitorNodeForceSum3Card card;
    card.name = m_name.get(row);
    card.label = m_label.get(row);
    card.outputAxes = m_outputAxes.get(row);
    card.nodeSetId = m_nodeSetId.get(row);
    card.elementSet = m_elementSet.get(row);
    card.positionFrameId = m_positionFrameId.get(row);
    card.xCoord = m_xCoord.get(row);
    card.yCoord = m_yCoord.get(row);
    card.zCoord = m_zCoord.get(row);
    card.excluded = m_excluded.get(row);
    card.displacementFrameId = m_displacementFrameId.get(row);
    return card;
}

void MonitorNodeForceSum3Table::store(std::uint32_t row, const MonitorNodeForceSum3Card& card)
{
    m_name.set(row, card.name);
    m_label.set(row, card.label);
    m_outputAxes.set(row, card.outputAxes);
    m_nodeSetId.set(row, card.nodeSetId);
    m_elementSet.set(row, card.elementSet);
    m_positionFrameId.set(row, card.positionFrameId);
    m_xCoord.set(row, card.xCoord);
    m_yCoord.set(row, card.yCoord);
    m_zCoord.set(row, card.zCoord);
    m_excluded.set(row, card.excluded);
    m_displacementFrameId.set(row, card.displacementFrameId);
}

void MonitorNodeForceSum3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_label.keepRows(rows);
    m_outputAxes.keepRows(rows);
    m_nodeSetId.keepRows(rows);
    m_elementSet.keepRows(rows);
    m_positionFrameId.keepRows(rows);
    m_xCoord.keepRows(rows);
    m_yCoord.keepRows(rows);
    m_zCoord.keepRows(rows);
    m_excluded.keepRows(rows);
    m_displacementFrameId.keepRows(rows);
}

void MonitorNodeForceSum3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId gridSet = m_nodeSetId.get(row); gridSet > 0)
        m_nodeSetId.set(row, visitor.visit("nodeSetId", ReferenceTarget::Set, gridSet));
    if (const base::EntityId elementSet = m_elementSet.get(row); elementSet > 0)
        m_elementSet.set(row, visitor.visit("elementSetId", ReferenceTarget::Set, elementSet));
    if (const base::EntityId cp = m_positionFrameId.get(row); cp > 0)
        m_positionFrameId.set(row, visitor.visit("positionFrameId", ReferenceTarget::Coordinate, cp));
    if (const base::EntityId displacementFrame = m_displacementFrameId.get(row); displacementFrame > 0)
        m_displacementFrameId.set(row,
                                  visitor.visit("displacementFrameId", ReferenceTarget::Coordinate, displacementFrame));
}

void MonitorNodeForceSum3Table::finishRead(Model&, const ReadContext& context)
{
    const std::string card = std::string(NAME);
    std::set<std::string> names;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row))
            continue;
        const std::string name = m_name.get(row);
        if (!names.insert(name).second && reported.insert(name).second)
            context.sink.error("deck.duplicate-monitor-node-force-sum3",
                               "the " + card + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
