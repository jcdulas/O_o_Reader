#include "oor/model/cards/matrices/monitorSumTotal/MonitorSumTotalTable.hpp"

#include "oor/base/text/DigitSets.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The line after the head holds AXES, CP, X, Y, Z and CD; the lines after that hold a type and up to seven names.
constexpr int FIELDS_PER_LINE = 8;
constexpr int NAMES_PER_LINE = 7;
constexpr int AXES_FIELD = 9;
constexpr int POSITION_FRAME_FIELD = 10;
constexpr int X_FIELD = 11;
constexpr int Y_FIELD = 12;
constexpr int Z_FIELD = 13;
constexpr int DISPLACEMENT_FRAME_FIELD = 14;
constexpr int LAST_LOCATION_FIELD = 16;
constexpr int FIRST_MEMBER_LINE = 2;

} // namespace

std::optional<MonitorSumTotalCard> MonitorSumTotalFormat::read(FieldReader& reader)
{
    MonitorSumTotalCard card;
    card.entryName = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.outputAxes = base::sortedDigits(reader.requiredInteger(AXES_FIELD, "outputAxes"));
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
                         "the second line of a " + std::string(reader.cardName()) + " has no field beyond " +
                             std::string("displacementFrameId"),
                         "displacementFrameId");
            break;
        }
    }
    // Each line: a type, then up to seven names. A blank type goes on with the one above.
    const int last = reader.lastUsedField();
    std::optional<MonitorResultType> type;
    for (int line = FIRST_MEMBER_LINE; line * FIELDS_PER_LINE < last && !reader.failed(); ++line)
    {
        const int lineBase = line * FIELDS_PER_LINE;
        if (!reader.isBlank(lineBase + 1))
        {
            type = readMonitorType(reader, lineBase + 1, "memberType", FORCE_MONITOR_TYPES);
            if (!type)
                break;
        }
        for (int field = lineBase + 2; field <= lineBase + FIELDS_PER_LINE; ++field)
        {
            if (reader.isBlank(field))
                continue;
            if (!type)
            {
                reader.error("card.missing-field",
                             "a " + std::string("memberNames") + " needs the " + std::string("memberType") +
                                 " of its line or of the line above",
                             "memberType");
                break;
            }
            card.members.push_back(MonitorSumTotalMember{*type, std::string(reader.text(field))});
        }
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorSumTotalFormat::check(const MonitorSumTotalCard& card)
{
    if (const auto problem = monitorNameProblem(card.entryName))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (card.outputAxes <= 0 || !base::isComponentSet(card.outputAxes))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " must be a set of the digits 1 to 6, each at most once",
                           "outputAxes"};
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
    if (card.members.empty())
        return CardProblem{"card.missing-field", "at least one monitor result to sum is required", "entryName"};
    for (const MonitorSumTotalMember& member : card.members)
    {
        if (const auto problem = monitorTypeProblem(member.type, FORCE_MONITOR_TYPES, "memberType"))
            return problem;
        if (const auto problem = monitoredNameProblem(member.name, "entryName"))
            return problem;
    }
    return std::nullopt;
}

void MonitorSumTotalFormat::write(const MonitorSumTotalCard& card, FieldWriter& writer)
{
    writer.text(1, card.entryName);
    writer.spanText(2, 7, card.label);
    writer.integer(AXES_FIELD, card.outputAxes);
    writer.integerUnlessDefault(POSITION_FRAME_FIELD, card.positionFrameId, 0);
    writer.realUnlessDefault(X_FIELD, card.xCoord, 0.0);
    writer.realUnlessDefault(Y_FIELD, card.yCoord, 0.0);
    writer.realUnlessDefault(Z_FIELD, card.zCoord, 0.0);
    writer.integer(DISPLACEMENT_FRAME_FIELD, card.displacementFrameId); // resolved: its default is CP
    // The members of one type in a row share its MTYP: it is written on their first line, and a line that goes on has
    // a blank one.
    int line = FIRST_MEMBER_LINE;
    std::size_t index = 0;
    while (index < card.members.size())
    {
        const MonitorResultType type = card.members[index].type;
        std::size_t end = index;
        while (end < card.members.size() && card.members[end].type == type)
            ++end;
        bool firstLine = true;
        while (index < end)
        {
            const int lineBase = line * FIELDS_PER_LINE;
            if (firstLine)
                writer.text(lineBase + 1, monitorTypeWord(type));
            for (int slot = 0; slot < NAMES_PER_LINE && index < end; ++slot, ++index)
                writer.text(lineBase + 2 + slot, card.members[index].name);
            firstLine = false;
            ++line;
        }
    }
}

std::uint32_t MonitorSumTotalTable::append(const MonitorSumTotalCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_cards.push(card);
    return row;
}

void MonitorSumTotalTable::store(std::uint32_t row, const MonitorSumTotalCard& card)
{
    m_entryName.set(row, card.entryName);
    m_cards.set(row, card);
}

void MonitorSumTotalTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_cards.keepRows(rows);
}

void MonitorSumTotalTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The system of the location and the system of the output; the basic system (0) is not a reference.
    MonitorSumTotalCard& card = m_cards.edit(row);
    visitId(visitor, "positionFrameId", ReferenceTarget::Coordinate, card.positionFrameId);
    visitId(visitor, "displacementFrameId", ReferenceTarget::Coordinate, card.displacementFrameId);
}

} // namespace oor::model
