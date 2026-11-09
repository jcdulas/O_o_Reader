#include "oor/model/cards/matrices/monitorSumLocated/MonitorSumLocatedTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The line after the head holds the location; the terms start on the next one.
constexpr int LOCATION_LINE = 1;
constexpr int FIRST_TERM_LINE = 2;
constexpr int POSITION_FRAME_FIELD = 9;
constexpr int X_FIELD = 10;
constexpr int Y_FIELD = 11;
constexpr int Z_FIELD = 12;
constexpr int DISPLACEMENT_FRAME_FIELD = 13;
constexpr int LAST_LOCATION_FIELD = 16;
constexpr const char* NEW_AXES_NAME = "newAxes";

} // namespace

std::optional<MonitorSumLocatedCard> MonitorSumLocatedFormat::read(FieldReader& reader)
{
    static_assert(LOCATION_LINE * 8 + 1 == POSITION_FRAME_FIELD);
    MonitorSumLocatedCard card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.positionFrameId = reader.integer(POSITION_FRAME_FIELD, "positionFrameId", 0);
    card.xCoord = reader.real(X_FIELD, "xCoord", 0.0);
    card.yCoord = reader.real(Y_FIELD, "yCoord", 0.0);
    card.zCoord = reader.real(Z_FIELD, "zCoord", 0.0);
    card.displacementFrameId = reader.integer(DISPLACEMENT_FRAME_FIELD, "displacementFrameId",
                                              card.positionFrameId); // the format reference: the system of CP
    for (int field = DISPLACEMENT_FRAME_FIELD + 1; field <= LAST_LOCATION_FIELD; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value",
                         "the location line of a " + std::string(reader.cardName()) + " has no field beyond " +
                             std::string("displacementFrameId"),
                         "displacementFrameId");
            break;
        }
    }
    card.body = readMonitorSumTerms(reader, FIRST_TERM_LINE, FORCE_MONITOR_TYPES, NEW_AXES_NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorSumLocatedFormat::check(const MonitorSumLocatedCard& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
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
    return checkMonitorSumTerms(card.body, FORCE_MONITOR_TYPES, NEW_AXES_NAME);
}

void MonitorSumLocatedFormat::write(const MonitorSumLocatedCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writer.integerUnlessDefault(POSITION_FRAME_FIELD, card.positionFrameId, 0);
    writer.realUnlessDefault(X_FIELD, card.xCoord, 0.0);
    writer.realUnlessDefault(Y_FIELD, card.yCoord, 0.0);
    writer.realUnlessDefault(Z_FIELD, card.zCoord, 0.0);
    writer.integer(DISPLACEMENT_FRAME_FIELD, card.displacementFrameId); // resolved: its default is CP
    writeMonitorSumTerms(writer, FIRST_TERM_LINE, card.body);
}

std::uint32_t MonitorSumLocatedTable::append(const MonitorSumLocatedCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void MonitorSumLocatedTable::store(std::uint32_t row, const MonitorSumLocatedCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void MonitorSumLocatedTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

void MonitorSumLocatedTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The system of the location and the system of the output; the basic system (0) is not a reference.
    MonitorSumLocatedCard& card = m_cards.edit(row);
    visitId(visitor, "positionFrameId", ReferenceTarget::Coordinate, card.positionFrameId);
    visitId(visitor, "displacementFrameId", ReferenceTarget::Coordinate, card.displacementFrameId);
}

} // namespace oor::model
