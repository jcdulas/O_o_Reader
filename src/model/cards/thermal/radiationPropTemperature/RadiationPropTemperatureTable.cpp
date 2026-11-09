#include "oor/model/cards/thermal/radiationPropTemperature/RadiationPropTemperatureTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int ABSORPTIVITY_FIELD = 2;
constexpr int FIRST_EMISSIVITY_FIELD = 3;

} // namespace

std::optional<RadiationPropTemperatureCard> RadiationPropTemperatureFormat::read(FieldReader& reader)
{
    RadiationPropTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.absorptivityTableId = reader.integer(ABSORPTIVITY_FIELD, "absorptivityTableId", 0);
    // One table for each band, from field 3 over as many lines as the card has. A blank field is a band without a
    // table, so the fields keep their places in the list; the trailing blanks are the end of the card.
    const int last = reader.lastUsedField();
    for (int field = FIRST_EMISSIVITY_FIELD; field <= last && !reader.failed(); ++field)
        card.emissivityTableIds.push_back(reader.integer(field, "emissivityTableId", 0));
    while (!card.emissivityTableIds.empty() && card.emissivityTableIds.back() == 0)
        card.emissivityTableIds.pop_back();
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RadiationPropTemperatureFormat::check(const RadiationPropTemperatureCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkTableReference(card.absorptivityTableId, "absorptivityTableId"))
        return problem;
    for (const base::EntityId table : card.emissivityTableIds)
    {
        if (const auto problem = checkTableReference(table, "emissivityTableId"))
            return problem;
    }
    return std::nullopt;
}

void RadiationPropTemperatureFormat::write(const RadiationPropTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    if (card.absorptivityTableId != 0)
        writer.integer(ABSORPTIVITY_FIELD, card.absorptivityTableId);
    int field = FIRST_EMISSIVITY_FIELD;
    for (const base::EntityId table : card.emissivityTableIds)
    {
        if (table != 0)
            writer.integer(field, table);
        ++field;
    }
}

std::uint32_t RadiationPropTemperatureTable::append(const RadiationPropTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_absorptivityTableId.push(card.absorptivityTableId);
    m_emissivityTableIds.push(card.emissivityTableIds);
    return row;
}

RadiationPropTemperatureCard RadiationPropTemperatureTable::card(std::uint32_t row) const
{
    RadiationPropTemperatureCard card;
    card.id = m_id.get(row);
    card.absorptivityTableId = m_absorptivityTableId.get(row);
    const auto tables = m_emissivityTableIds.get(row);
    card.emissivityTableIds.assign(tables.begin(), tables.end());
    return card;
}

void RadiationPropTemperatureTable::store(std::uint32_t row, const RadiationPropTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_absorptivityTableId.set(row, card.absorptivityTableId);
    m_emissivityTableIds.set(row, card.emissivityTableIds);
}

void RadiationPropTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_absorptivityTableId.keepRows(rows);
    m_emissivityTableIds.keepRows(rows);
}

void RadiationPropTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the RADM that the card extends; the tables are TABLEMj cards. A blank field (0) is no table.
    m_id.set(row, visitor.visit("id", ReferenceTarget::RadiationMaterial, m_id.get(row)));
    if (const base::EntityId table = m_absorptivityTableId.get(row); table > 0)
        m_absorptivityTableId.set(row, visitor.visit("absorptivityTableId", ReferenceTarget::MaterialTable, table));
    const auto current = m_emissivityTableIds.get(row);
    std::vector<base::EntityId> tables(current.begin(), current.end());
    bool changed = false;
    for (base::EntityId& table : tables)
    {
        if (table <= 0)
            continue;
        const base::EntityId updated = visitor.visit("emissivityTableId", ReferenceTarget::MaterialTable, table);
        changed = changed || updated != table;
        table = updated;
    }
    if (changed)
        m_emissivityTableIds.set(row, tables);
}

} // namespace oor::model
