#include "oor/model/cards/materials/anisoMat2DTemperature/AnisoMat2DTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr const char* STIFFNESS_TABLE_NAMES[6] = {"stiffness11TableId", "stiffness12TableId", "stiffness13TableId",
                                                  "stiffness22TableId", "stiffness23TableId", "stiffness33TableId"};
constexpr const char* THERMAL_EXPANSION_TABLE_NAMES[3] = {"thermalExpansion1TableId", "thermalExpansion2TableId",
                                                          "thermalExpansion3TableId"};
constexpr const char* DAMPING_TERM_TABLE_NAMES[6] = {"structuralDamping11TableId", "structuralDamping12TableId",
                                                     "structuralDamping13TableId", "structuralDamping22TableId",
                                                     "structuralDamping23TableId", "structuralDamping33TableId"};

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

} // namespace

std::optional<AnisoMat2DTemperatureCard> AnisoMat2DTemperatureFormat::read(FieldReader& reader)
{
    AnisoMat2DTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    for (std::size_t index = 0; index < 6; ++index)
        card.stiffnessTableIds[index] = reader.integer(2 + static_cast<int>(index), STIFFNESS_TABLE_NAMES[index], 0);
    card.densityTableId = reader.integer(8, "densityTableId", 0);
    for (std::size_t index = 0; index < 3; ++index)
        card.thermalExpansionTableIds[index] =
            reader.integer(9 + static_cast<int>(index), THERMAL_EXPANSION_TABLE_NAMES[index], 0);
    card.structuralDampingTableId = reader.integer(13, "structuralDampingTableId", 0);
    card.tensionAllowableTableId = reader.integer(14, "tensionAllowableTableId", 0);
    card.compressionAllowableTableId = reader.integer(15, "compressionAllowableTableId", 0);
    card.shearAllowableTableId = reader.integer(16, "shearAllowableTableId", 0);
    for (std::size_t index = 0; index < 6; ++index)
        card.structuralDampingTermTableIds[index] =
            reader.integer(18 + static_cast<int>(index), DAMPING_TERM_TABLE_NAMES[index], 0);
    requireBlankFields(reader, {12, 17}, 23);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AnisoMat2DTemperatureFormat::check(const AnisoMat2DTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    for (std::size_t index = 0; index < 6; ++index)
    {
        if (const auto problem = checkTableReference(card.stiffnessTableIds[index], STIFFNESS_TABLE_NAMES[index]))
            return problem;
    }
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    for (std::size_t index = 0; index < 3; ++index)
    {
        if (const auto problem =
                checkTableReference(card.thermalExpansionTableIds[index], THERMAL_EXPANSION_TABLE_NAMES[index], true))
            return problem;
    }
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensionAllowableTableId, "tensionAllowableTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionAllowableTableId, "compressionAllowableTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.shearAllowableTableId, "shearAllowableTableId"))
        return problem;
    for (std::size_t index = 0; index < 6; ++index)
    {
        if (const auto problem =
                checkTableReference(card.structuralDampingTermTableIds[index], DAMPING_TERM_TABLE_NAMES[index]))
            return problem;
    }
    return std::nullopt;
}

void AnisoMat2DTemperatureFormat::write(const AnisoMat2DTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    for (std::size_t index = 0; index < 6; ++index)
        writer.integerUnlessDefault(2 + static_cast<int>(index), card.stiffnessTableIds[index], 0);
    writer.integerUnlessDefault(8, card.densityTableId, 0);
    for (std::size_t index = 0; index < 3; ++index)
        writer.integerUnlessDefault(9 + static_cast<int>(index), card.thermalExpansionTableIds[index], 0);
    writer.integerUnlessDefault(13, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(14, card.tensionAllowableTableId, 0);
    writer.integerUnlessDefault(15, card.compressionAllowableTableId, 0);
    writer.integerUnlessDefault(16, card.shearAllowableTableId, 0);
    for (std::size_t index = 0; index < 6; ++index)
        writer.integerUnlessDefault(18 + static_cast<int>(index), card.structuralDampingTermTableIds[index], 0);
}

std::uint32_t AnisoMat2DTemperatureTable::append(const AnisoMat2DTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_stiffnessTableIds.push(card.stiffnessTableIds);
    m_densityTableId.push(card.densityTableId);
    m_thermalExpansionTableIds.push(card.thermalExpansionTableIds);
    m_structuralDampingTableId.push(card.structuralDampingTableId);
    m_tensionAllowableTableId.push(card.tensionAllowableTableId);
    m_compressionAllowableTableId.push(card.compressionAllowableTableId);
    m_shearAllowableTableId.push(card.shearAllowableTableId);
    m_structuralDampingTermTableIds.push(card.structuralDampingTermTableIds);
    return row;
}

AnisoMat2DTemperatureCard AnisoMat2DTemperatureTable::card(std::uint32_t row) const
{
    AnisoMat2DTemperatureCard card;
    card.id = m_id.get(row);
    card.stiffnessTableIds = m_stiffnessTableIds.get(row);
    card.densityTableId = m_densityTableId.get(row);
    card.thermalExpansionTableIds = m_thermalExpansionTableIds.get(row);
    card.structuralDampingTableId = m_structuralDampingTableId.get(row);
    card.tensionAllowableTableId = m_tensionAllowableTableId.get(row);
    card.compressionAllowableTableId = m_compressionAllowableTableId.get(row);
    card.shearAllowableTableId = m_shearAllowableTableId.get(row);
    card.structuralDampingTermTableIds = m_structuralDampingTermTableIds.get(row);
    return card;
}

void AnisoMat2DTemperatureTable::store(std::uint32_t row, const AnisoMat2DTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_stiffnessTableIds.set(row, card.stiffnessTableIds);
    m_densityTableId.set(row, card.densityTableId);
    m_thermalExpansionTableIds.set(row, card.thermalExpansionTableIds);
    m_structuralDampingTableId.set(row, card.structuralDampingTableId);
    m_tensionAllowableTableId.set(row, card.tensionAllowableTableId);
    m_compressionAllowableTableId.set(row, card.compressionAllowableTableId);
    m_shearAllowableTableId.set(row, card.shearAllowableTableId);
    m_structuralDampingTermTableIds.set(row, card.structuralDampingTermTableIds);
}

void AnisoMat2DTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_stiffnessTableIds.keepRows(rows);
    m_densityTableId.keepRows(rows);
    m_thermalExpansionTableIds.keepRows(rows);
    m_structuralDampingTableId.keepRows(rows);
    m_tensionAllowableTableId.keepRows(rows);
    m_compressionAllowableTableId.keepRows(rows);
    m_shearAllowableTableId.keepRows(rows);
    m_structuralDampingTermTableIds.keepRows(rows);
}

void AnisoMat2DTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MAT2 that the card extends; the tables are TABLEMi cards (a negative T(Ai) is the table
    // of thermal strains of that number).
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    std::array<base::EntityId, 6> stiffnessTables = m_stiffnessTableIds.get(row);
    if (visitTables(visitor, STIFFNESS_TABLE_NAMES, stiffnessTables))
        m_stiffnessTableIds.set(row, stiffnessTables);
    if (const base::EntityId id = m_densityTableId.get(row); id != 0)
        m_densityTableId.set(row, visitor.visit("densityTableId", ReferenceTarget::MaterialTable, id));
    std::array<base::EntityId, 3> thermalExpansionTables = m_thermalExpansionTableIds.get(row);
    if (visitTables(visitor, THERMAL_EXPANSION_TABLE_NAMES, thermalExpansionTables))
        m_thermalExpansionTableIds.set(row, thermalExpansionTables);
    if (const base::EntityId id = m_structuralDampingTableId.get(row); id != 0)
        m_structuralDampingTableId.set(row,
                                       visitor.visit("structuralDampingTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_tensionAllowableTableId.get(row); id != 0)
        m_tensionAllowableTableId.set(row,
                                      visitor.visit("tensionAllowableTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_compressionAllowableTableId.get(row); id != 0)
        m_compressionAllowableTableId.set(
            row, visitor.visit("compressionAllowableTableId", ReferenceTarget::MaterialTable, id));
    if (const base::EntityId id = m_shearAllowableTableId.get(row); id != 0)
        m_shearAllowableTableId.set(row, visitor.visit("shearAllowableTableId", ReferenceTarget::MaterialTable, id));
    std::array<base::EntityId, 6> dampingTermTables = m_structuralDampingTermTableIds.get(row);
    if (visitTables(visitor, DAMPING_TERM_TABLE_NAMES, dampingTermTables))
        m_structuralDampingTermTableIds.set(row, dampingTermTables);
}

} // namespace oor::model
