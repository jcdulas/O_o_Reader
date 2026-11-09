#include "oor/model/cards/sets/radiatedPowerPanelSet/RadiatedPowerPanelSetTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>
#include <utility>

namespace oor::model
{

std::optional<RadiatedPowerPanelSetCard> RadiatedPowerPanelSetFormat::read(FieldReader& reader)
{
    RadiatedPowerPanelSetCard card;
    // The pairs follow each other, label first, four to a line; a pair whose two fields are blank is padding.
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last; field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        RadiatedPowerPanel panel;
        if (reader.isBlank(field))
            reader.error("card.missing-field", std::string("panelName") + " is required", "panelName");
        panel.name = std::string(reader.text(field));
        panel.setId = reader.requiredInteger(field + 1, "panelSetId");
        card.panels.push_back(std::move(panel));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiatedPowerPanelSetFormat::check(const RadiatedPowerPanelSetCard& card)
{
    if (card.panels.empty())
        return CardProblem{"card.missing-field", "the card needs at least one panel", "panelName1"};
    for (const RadiatedPowerPanel& panel : card.panels)
    {
        if (const auto problem = checkNameWord(panel.name, "panelName"))
            return problem;
        if (const auto problem = checkRequiredId(panel.setId, "panelSetId"))
            return problem;
    }
    return std::nullopt;
}

void RadiatedPowerPanelSetFormat::write(const RadiatedPowerPanelSetCard& card, FieldWriter& writer)
{
    int field = 1;
    for (const RadiatedPowerPanel& panel : card.panels)
    {
        writer.text(field++, panel.name);
        writer.integer(field++, panel.setId);
    }
}

std::uint32_t RadiatedPowerPanelSetTable::append(const RadiatedPowerPanelSetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_panelCount.push(static_cast<std::int32_t>(card.panels.size()));
    m_cards.push(card);
    return row;
}

void RadiatedPowerPanelSetTable::store(std::uint32_t row, const RadiatedPowerPanelSetCard& card)
{
    m_panelCount.set(row, static_cast<std::int32_t>(card.panels.size()));
    m_cards.set(row, card);
}

void RadiatedPowerPanelSetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_panelCount.keepRows(rows);
    m_cards.keepRows(rows);
}

void RadiatedPowerPanelSetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    for (RadiatedPowerPanel& panel : m_cards.edit(row).panels)
        visitId(visitor, "panelSetId", ReferenceTarget::Set, panel.setId);
}

} // namespace oor::model
