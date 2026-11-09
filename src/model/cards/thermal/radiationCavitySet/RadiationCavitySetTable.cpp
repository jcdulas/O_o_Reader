#include "oor/model/cards/thermal/radiationCavitySet/RadiationCavitySetTable.hpp"

namespace oor::model
{

std::optional<RadiationCavitySetCard> RadiationCavitySetFormat::read(FieldReader& reader)
{
    RadiationCavitySetCard card;
    const int last = reader.lastUsedField();
    for (int field = 1; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        card.cavities.push_back(reader.requiredInteger(field, "cavityId"));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationCavitySetFormat::check(const RadiationCavitySetCard& card)
{
    if (card.cavities.empty())
        return CardProblem{"card.missing-field", "the card needs at least one cavity", "cavityId"};
    for (const base::EntityId cavity : card.cavities)
    {
        if (cavity <= 0)
            return CardProblem{"card.bad-id", "a cavity ID must be a positive integer", "cavityId"};
    }
    return std::nullopt;
}

void RadiationCavitySetFormat::write(const RadiationCavitySetCard& card, FieldWriter& writer)
{
    for (std::size_t index = 0; index < card.cavities.size(); ++index)
        writer.integer(1 + static_cast<int>(index), card.cavities[index]);
}

std::uint32_t RadiationCavitySetTable::append(const RadiationCavitySetCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cavities.push(card.cavities);
    return row;
}

RadiationCavitySetCard RadiationCavitySetTable::card(std::uint32_t row) const
{
    RadiationCavitySetCard card;
    const auto cavities = m_cavities.get(row);
    card.cavities.assign(cavities.begin(), cavities.end());
    return card;
}

void RadiationCavitySetTable::store(std::uint32_t row, const RadiationCavitySetCard& card)
{
    m_cavities.set(row, card.cavities);
}

void RadiationCavitySetTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_cavities.keepRows(rows);
}

void RadiationCavitySetTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const auto current = m_cavities.get(row);
    std::vector<base::EntityId> cavities(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& cavity : cavities)
    {
        const base::EntityId updated = visitor.visit("cavityId", ReferenceTarget::RadiationCavity, cavity);
        changed = changed || updated != cavity;
        cavity = updated;
    }
    if (changed)
        m_cavities.set(row, cavities);
}

} // namespace oor::model
