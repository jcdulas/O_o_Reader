#include "oor/model/cards/materials/anisoMat3DTemperature/AnisoMat3DTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_STIFFNESS_FIELD = 2;
constexpr int DENSITY_FIELD = 23;
constexpr int FIRST_THERMAL_EXPANSION_FIELD = 24;
constexpr int DAMPING_FIELD = 31;
constexpr int FIRST_DAMPING_TERM_FIELD = 33;
constexpr int LAST_FIELD = 53;

constexpr const char* STIFFNESS_TABLE_NAMES[21] = {
    "stiffness11TableId", "stiffness12TableId", "stiffness13TableId", "stiffness14TableId", "stiffness15TableId",
    "stiffness16TableId", "stiffness22TableId", "stiffness23TableId", "stiffness24TableId", "stiffness25TableId",
    "stiffness26TableId", "stiffness33TableId", "stiffness34TableId", "stiffness35TableId", "stiffness36TableId",
    "stiffness44TableId", "stiffness45TableId", "stiffness46TableId", "stiffness55TableId", "stiffness56TableId",
    "stiffness66TableId"};
constexpr const char* THERMAL_EXPANSION_TABLE_NAMES[6] = {"thermalExpansion1TableId", "thermalExpansion2TableId",
                                                          "thermalExpansion3TableId", "thermalExpansion4TableId",
                                                          "thermalExpansion5TableId", "thermalExpansion6TableId"};
constexpr const char* DAMPING_TERM_TABLE_NAMES[21] = {
    "structuralDamping11TableId", "structuralDamping12TableId", "structuralDamping13TableId",
    "structuralDamping14TableId", "structuralDamping15TableId", "structuralDamping16TableId",
    "structuralDamping22TableId", "structuralDamping23TableId", "structuralDamping24TableId",
    "structuralDamping25TableId", "structuralDamping26TableId", "structuralDamping33TableId",
    "structuralDamping34TableId", "structuralDamping35TableId", "structuralDamping36TableId",
    "structuralDamping44TableId", "structuralDamping45TableId", "structuralDamping46TableId",
    "structuralDamping55TableId", "structuralDamping56TableId", "structuralDamping66TableId"};

// Visits the table IDs of a group; returns true when one of them changed.
template <std::size_t N>
bool visitTables(ReferenceVisitor& visitor, const char* const (&names)[N], std::array<base::EntityId, N>& tables)
{
    bool changed = false;
    for (std::size_t index = 0; index < N; ++index)
    {
        if (tables[index] == 0)
            continue;
        const base::EntityId updated = visitor.visit(names[index], ReferenceTarget::MaterialTable, tables[index]);
        changed = changed || updated != tables[index];
        tables[index] = updated;
    }
    return changed;
}

template <std::size_t N>
std::optional<CardProblem> checkTables(const std::array<base::EntityId, N>& tables, const char* const (&names)[N])
{
    for (std::size_t index = 0; index < N; ++index)
    {
        if (const auto problem = checkTableReference(tables[index], names[index]))
            return problem;
    }
    return std::nullopt;
}

} // namespace

std::optional<AnisoMat3DTemperatureCard> AnisoMat3DTemperatureFormat::read(FieldReader& reader)
{
    AnisoMat3DTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < 21; ++index)
        card.stiffnessTableIds[index] =
            reader.integer(FIRST_STIFFNESS_FIELD + static_cast<int>(index), STIFFNESS_TABLE_NAMES[index], 0);
    card.densityTableId = reader.integer(DENSITY_FIELD, "densityTableId", 0);
    for (std::size_t index = 0; index < 6; ++index)
        card.thermalExpansionTableIds[index] = reader.integer(FIRST_THERMAL_EXPANSION_FIELD + static_cast<int>(index),
                                                              THERMAL_EXPANSION_TABLE_NAMES[index], 0);
    card.structuralDampingTableId = reader.integer(DAMPING_FIELD, "structuralDampingTableId", 0);
    for (std::size_t index = 0; index < 21; ++index)
        card.structuralDampingTermTableIds[index] =
            reader.integer(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index), DAMPING_TERM_TABLE_NAMES[index], 0);
    requireBlankFields(reader, {30, 32}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat3DTemperatureFormat::check(const AnisoMat3DTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTables(card.stiffnessTableIds, STIFFNESS_TABLE_NAMES))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    if (const auto problem = checkTables(card.thermalExpansionTableIds, THERMAL_EXPANSION_TABLE_NAMES))
        return problem;
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    return checkTables(card.structuralDampingTermTableIds, DAMPING_TERM_TABLE_NAMES);
}

void AnisoMat3DTemperatureFormat::write(const AnisoMat3DTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < 21; ++index)
        writer.integerUnlessDefault(FIRST_STIFFNESS_FIELD + static_cast<int>(index), card.stiffnessTableIds[index], 0);
    writer.integerUnlessDefault(DENSITY_FIELD, card.densityTableId, 0);
    for (std::size_t index = 0; index < 6; ++index)
        writer.integerUnlessDefault(FIRST_THERMAL_EXPANSION_FIELD + static_cast<int>(index),
                                    card.thermalExpansionTableIds[index], 0);
    writer.integerUnlessDefault(DAMPING_FIELD, card.structuralDampingTableId, 0);
    for (std::size_t index = 0; index < 21; ++index)
        writer.integerUnlessDefault(FIRST_DAMPING_TERM_FIELD + static_cast<int>(index),
                                    card.structuralDampingTermTableIds[index], 0);
}

std::uint32_t AnisoMat3DTemperatureTable::append(const AnisoMat3DTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffnessTableIds.push(card.stiffnessTableIds);
    m_densityTableId.push(card.densityTableId);
    m_thermalExpansionTableIds.push(card.thermalExpansionTableIds);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    m_structuralDampingTermTableIds.push(card.structuralDampingTermTableIds);
    return row;
}

AnisoMat3DTemperatureCard AnisoMat3DTemperatureTable::card(std::uint32_t row) const
{
    AnisoMat3DTemperatureCard card;
    card.id = m_id.get(row);
    card.stiffnessTableIds = m_stiffnessTableIds.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.thermalExpansionTableIds = m_thermalExpansionTableIds.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    card.structuralDampingTermTableIds = m_structuralDampingTermTableIds.get(row);
    return card;
}

void AnisoMat3DTemperatureTable::store(std::uint32_t row, const AnisoMat3DTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_stiffnessTableIds.set(row, card.stiffnessTableIds);
    m_densityTableId.set(row, card.densityTableId);
    m_thermalExpansionTableIds.set(row, card.thermalExpansionTableIds);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
    m_structuralDampingTermTableIds.set(row, card.structuralDampingTermTableIds);
}

void AnisoMat3DTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffnessTableIds.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_thermalExpansionTableIds.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
    m_structuralDampingTermTableIds.keepRows(rows);
}

void AnisoMat3DTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT9 that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    std::array<base::EntityId, 21> stiffnessTables = m_stiffnessTableIds.get(row);
    if (visitTables(visitor, STIFFNESS_TABLE_NAMES, stiffnessTables))
        m_stiffnessTableIds.set(row, stiffnessTables);
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    std::array<base::EntityId, 6> thermalExpansionTables = m_thermalExpansionTableIds.get(row);
    if (visitTables(visitor, THERMAL_EXPANSION_TABLE_NAMES, thermalExpansionTables))
        m_thermalExpansionTableIds.set(row, thermalExpansionTables);
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
    std::array<base::EntityId, 21> dampingTermTables = m_structuralDampingTermTableIds.get(row);
    if (visitTables(visitor, DAMPING_TERM_TABLE_NAMES, dampingTermTables))
        m_structuralDampingTermTableIds.set(row, dampingTermTables);
}

} // namespace oor::model
