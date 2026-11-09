#include "oor/model/cards/matrices/monitorElemLoad2/MonitorElemLoad2Table.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/matrices/MonitorPointFields.hpp"
#include "oor/model/core/Model.hpp"

#include <set>

namespace oor::model
{

std::optional<MonitorElemLoad2Card> MonitorElemLoad2Format::read(FieldReader& reader)
{
    MonitorElemLoad2Card card;
    card.name = std::string(reader.text(1));
    card.label = reader.spanText(2, 8);
    card.resultTable = std::string(reader.text(9));
    card.kind = readCardTypeWord(reader, 10);
    card.resultItem = std::string(reader.text(11));
    card.elemId = reader.integer(12, "elemId", 0);
    if (reader.lastUsedField() > 12)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("elemId"), "elemId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MonitorElemLoad2Format::check(const MonitorElemLoad2Card& card)
{
    if (const auto problem = monitorNameProblem(card.name))
        return problem;
    if (const auto problem = monitorLabelProblem(card.label))
        return problem;
    if (card.resultTable != "Stress" && card.resultTable != "Force" && card.resultTable != "Strain")
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be Stress, Force or Strain",
                           "resultTable"};
    if (card.kind.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " must name the element type",
                           "kind"};
    if (card.resultItem.empty())
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " must name the component to monitor", "resultItem"};
    if (card.elemId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "elemId"};
    return std::nullopt;
}

void MonitorElemLoad2Format::write(const MonitorElemLoad2Card& card, FieldWriter& writer)
{
    writer.text(1, card.name);
    writer.spanText(2, 7, card.label);
    writer.text(9, card.resultTable);
    writer.text(10, card.kind);
    writer.text(11, card.resultItem);
    writer.integerUnlessDefault(12, card.elemId, 0);
}

std::uint32_t MonitorElemLoad2Table::append(const MonitorElemLoad2Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_name.push(card.name);
    m_label.push(card.label);
    m_resultTable.push(card.resultTable);
    m_kind.push(card.kind);
    m_resultItem.push(card.resultItem);
    m_elemId.push(card.elemId);
    return row;
}

MonitorElemLoad2Card MonitorElemLoad2Table::card(std::uint32_t row) const
{
    MonitorElemLoad2Card card;
    card.name = m_name.get(row);
    card.label = m_label.get(row);
    card.resultTable = m_resultTable.get(row);
    card.kind = m_kind.get(row);
    card.resultItem = m_resultItem.get(row);
    card.elemId = m_elemId.get(row);
    return card;
}

void MonitorElemLoad2Table::store(std::uint32_t row, const MonitorElemLoad2Card& card)
{
    m_name.set(row, card.name);
    m_label.set(row, card.label);
    m_resultTable.set(row, card.resultTable);
    m_kind.set(row, card.kind);
    m_resultItem.set(row, card.resultItem);
    m_elemId.set(row, card.elemId);
}

void MonitorElemLoad2Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_name.keepRows(rows);
    m_label.keepRows(rows);
    m_resultTable.keepRows(rows);
    m_kind.keepRows(rows);
    m_resultItem.keepRows(rows);
    m_elemId.keepRows(rows);
}

void MonitorElemLoad2Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The format reference ignores an element that does not exist (a fictitious EID gives a solver warning): the
    // reference may be missing, as the element of a THRU list may.
    if (const base::EntityId element = m_elemId.get(row); element > 0)
        m_elemId.set(row, visitor.visit("elemId", ReferenceTarget::ElementInRange, element));
}

void MonitorElemLoad2Table::finishRead(Model&, const ReadContext& context)
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
            context.sink.error("deck.duplicate-monitor-elem-load2",
                               "the " + card + " name " + name + " is used by more than one card");
    }
}

} // namespace oor::model
