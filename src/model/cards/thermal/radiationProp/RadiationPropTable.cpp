#include "oor/model/cards/thermal/radiationProp/RadiationPropTable.hpp"

#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

double notGiven()
{
    return std::numeric_limits<double>::quiet_NaN();
}

} // namespace

RadiationPropTable::RadiationPropTable() : m_absorptivity(notGiven()) {}

std::optional<RadiationPropCard> RadiationPropFormat::read(FieldReader& reader)
{
    RadiationPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.absorptivity = reader.optionalReal(2, "absorptivity");
    // The emissivities follow, one per field, over as many lines as the card has (blank padding fields are skipped).
    const int last = reader.lastUsedField();
    for (int field = 3; field <= last && !reader.failed(); ++field)
    {
        if (reader.isBlank(field))
            continue;
        const std::string name = "EMIS" + std::to_string(card.emissivity.size() + 1);
        card.emissivity.push_back(reader.requiredReal(field, name.c_str()));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationPropFormat::check(const RadiationPropCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the radiation material ID must be a positive integer", "id"};
    if (card.absorptivity && !(*card.absorptivity >= 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " cannot be negative", "absorptivity"};
    for (const double emissivity : card.emissivity)
    {
        if (!(emissivity >= 0.0))
            return CardProblem{"card.bad-value", "an emissivity cannot be negative", "emissivity"};
    }
    return std::nullopt;
}

void RadiationPropFormat::write(const RadiationPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.absorptivity)
        writer.real(2, *card.absorptivity);
    for (std::size_t index = 0; index < card.emissivity.size(); ++index)
        writer.real(3 + static_cast<int>(index), card.emissivity[index]);
}

std::uint32_t RadiationPropTable::append(const RadiationPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_absorptivity.push(card.absorptivity.value_or(notGiven()));
    m_emissivity.push(card.emissivity);
    return row;
}

RadiationPropCard RadiationPropTable::card(std::uint32_t row) const
{
    RadiationPropCard card;
    card.id = m_id.get(row);
    if (const double absorptivity = m_absorptivity.get(row); !std::isnan(absorptivity))
        card.absorptivity = absorptivity;
    const auto emissivity = m_emissivity.get(row);
    card.emissivity.assign(emissivity.begin(), emissivity.end());
    return card;
}

void RadiationPropTable::store(std::uint32_t row, const RadiationPropCard& card)
{
    m_id.set(row, card.id);
    m_absorptivity.set(row, card.absorptivity.value_or(notGiven()));
    m_emissivity.set(row, card.emissivity);
}

void RadiationPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_absorptivity.keepRows(rows);
    m_emissivity.keepRows(rows);
}

} // namespace oor::model
