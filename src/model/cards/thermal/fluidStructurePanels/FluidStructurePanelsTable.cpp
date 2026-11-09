#include "oor/model/cards/thermal/fluidStructurePanels/FluidStructurePanelsTable.hpp"

namespace oor::model
{

std::optional<FluidStructurePanelsCard> FluidStructurePanelsFormat::read(FieldReader& reader)
{
    FluidStructurePanelsCard card;
    const int last = reader.lastUsedField();
    // The pairs of (name, set), over as many lines as the card has; a blank pair is skipped (the padding of a line).
    for (int field = 1; field <= last && !reader.failed(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        if (reader.isBlank(field))
        {
            reader.error("card.missing-field", "a panel needs its " + std::string("panelName"), "panelName");
            break;
        }
        card.names.push_back(std::string(reader.text(field)));
        card.sets.push_back(reader.requiredInteger(field + 1, "panelSetId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FluidStructurePanelsFormat::check(const FluidStructurePanelsCard& card)
{
    if (card.names.empty())
        return CardProblem{"card.missing-field", "the card needs at least one panel", "panelName"};
    if (card.names.size() != card.sets.size())
        return CardProblem{"card.bad-value", "the card has one set ID for each name", "panelSetId"};
    for (std::size_t index = 0; index < card.names.size(); ++index)
    {
        if (card.names[index].empty())
            return CardProblem{"card.missing-field", "the name of a panel cannot be blank", "panelName"};
        if (card.sets[index] <= 0)
            return CardProblem{"card.bad-id", "a set ID must be a positive integer", "panelSetId"};
    }
    return std::nullopt;
}

void FluidStructurePanelsFormat::write(const FluidStructurePanelsCard& card, FieldWriter& writer)
{
    for (std::size_t index = 0; index < card.names.size(); ++index)
    {
        writer.text(1 + 2 * static_cast<int>(index), card.names[index]);
        writer.integer(2 + 2 * static_cast<int>(index), card.sets[index]);
    }
}

std::uint32_t FluidStructurePanelsTable::append(const FluidStructurePanelsCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_names.push(card.names);
    m_sets.push(card.sets);
    return row;
}

FluidStructurePanelsCard FluidStructurePanelsTable::card(std::uint32_t row) const
{
    FluidStructurePanelsCard card;
    const auto names = m_names.get(row);
    card.names.assign(names.begin(), names.end());
    const auto sets = m_sets.get(row);
    card.sets.assign(sets.begin(), sets.end());
    return card;
}

void FluidStructurePanelsTable::store(std::uint32_t row, const FluidStructurePanelsCard& card)
{
    m_names.set(row, card.names);
    m_sets.set(row, card.sets);
}

void FluidStructurePanelsTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_names.keepRows(rows);
    m_sets.keepRows(rows);
}

void FluidStructurePanelsTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto current = m_sets.get(row);
    std::vector<base::EntityId> sets(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& set : sets)
    {
        if (set <= 0)
            continue;
        const base::EntityId updated = visitor.visit("panelSetId", ReferenceTarget::Set, set);
        changed = changed || updated != set;
        set = updated;
    }
    if (changed)
        m_sets.set(row, sets);
}

} // namespace oor::model
