#include "oor/model/cards/matrices/monitorSum/MonitorSumTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_TERM_LINE = 1; // the line after the head
constexpr const char* NEW_AXES_NAME = "newAxes";

} // namespace

std::optional<MonitorSumCard> MonitorSumFormat::read(FieldReader& reader)
{
    MonitorSumCard card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.body = readMonitorSumTerms(reader, FIRST_TERM_LINE, ALL_MONITOR_TYPES, NEW_AXES_NAME);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorSumFormat::check(const MonitorSumCard& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    return checkMonitorSumTerms(card.body, ALL_MONITOR_TYPES, NEW_AXES_NAME);
}

void MonitorSumFormat::write(const MonitorSumCard& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writeMonitorSumTerms(writer, FIRST_TERM_LINE, card.body);
}

std::uint32_t MonitorSumTable::append(const MonitorSumCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_cards.push(card);
    return row;
}

void MonitorSumTable::store(std::uint32_t row, const MonitorSumCard& card)
{
    m_name.set(row, card.name);
    m_cards.set(row, card);
}

void MonitorSumTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_cards.keepRows(rows);
}

} // namespace oor::model
