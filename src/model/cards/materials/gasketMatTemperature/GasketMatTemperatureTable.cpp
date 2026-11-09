#include "oor/model/cards/materials/gasketMatTemperature/GasketMatTemperatureTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

// The name of the code of each unloading path field.
constexpr const char* UNLOADING_NAMES[GasketMatTemperatureCard::UNLOADING_TABLES] = {
    "unloadingTableId1", "unloadingTableId2", "unloadingTableId3", "unloadingTableId4", "unloadingTableId5",
    "unloadingTableId6", "unloadingTableId7", "unloadingTableId8", "unloadingTableId9", "unloadingTableId10"};

// IDU1 to IDU3 are the fields 6 to 8 and IDU4 to IDU10 the fields 9 to 15: ten in a row from field 6.
constexpr int FIRST_UNLOADING_FIELD = 6;

int unloadingField(std::size_t index)
{
    return FIRST_UNLOADING_FIELD + static_cast<int>(index);
}

} // namespace

std::optional<GasketMatTemperatureCard> GasketMatTemperatureFormat::read(FieldReader& reader)
{
    GasketMatTemperatureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.membraneYoungsModulusTableId = reader.integer(2, "membraneYoungsModulusTableId", 0);
    card.membranePoissonRatioTableId = reader.integer(3, "membranePoissonRatioTableId", 0);
    card.membraneDensityTableId = reader.integer(4, "membraneDensityTableId", 0);
    card.loadingTableId = reader.integer(5, "loadingTableId", 0);
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
        card.unloadingTableIds[index] = reader.integer(unloadingField(index), UNLOADING_NAMES[index], 0);
    card.yieldPressureTableId = reader.integer(16, "yieldPressureTableId", 0);
    card.tensileModulusTableId = reader.integer(17, "tensileModulusTableId", 0);
    card.transverseShearModulusTableId = reader.integer(18, "transverseShearModulusTableId", 0);
    card.initialGapTableId = reader.integer(19, "initialGapTableId", 0);
    requireBlankFields(reader, {}, 19);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasketMatTemperatureFormat::check(const GasketMatTemperatureCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.membraneYoungsModulusTableId, "membraneYoungsModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.membranePoissonRatioTableId, "membranePoissonRatioTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.membraneDensityTableId, "membraneDensityTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.loadingTableId, "loadingTableId"))
        return problem;
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
    {
        if (const auto problem = checkTableReference(card.unloadingTableIds[index], UNLOADING_NAMES[index]))
            return problem;
    }
    if (const auto problem = checkTableReference(card.yieldPressureTableId, "yieldPressureTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensileModulusTableId, "tensileModulusTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.transverseShearModulusTableId, "transverseShearModulusTableId"))
        return problem;
    return checkTableReference(card.initialGapTableId, "initialGapTableId");
}

void GasketMatTemperatureFormat::write(const GasketMatTemperatureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.membraneYoungsModulusTableId, 0);
    writer.integerUnlessDefault(3, card.membranePoissonRatioTableId, 0);
    writer.integerUnlessDefault(4, card.membraneDensityTableId, 0);
    writer.integerUnlessDefault(5, card.loadingTableId, 0);
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
        writer.integerUnlessDefault(unloadingField(index), card.unloadingTableIds[index], 0);
    writer.integerUnlessDefault(16, card.yieldPressureTableId, 0);
    writer.integerUnlessDefault(17, card.tensileModulusTableId, 0);
    writer.integerUnlessDefault(18, card.transverseShearModulusTableId, 0);
    writer.integerUnlessDefault(19, card.initialGapTableId, 0);
}

std::uint32_t GasketMatTemperatureTable::append(const GasketMatTemperatureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_membraneYoungsModulusTableId.push(card.membraneYoungsModulusTableId);
    m_membranePoissonRatioTableId.push(card.membranePoissonRatioTableId);
    m_membraneDensityTableId.push(card.membraneDensityTableId);
    m_loadingTableId.push(card.loadingTableId);
    m_unloadingTableIds.push(card.unloadingTableIds);
    m_yieldPressureTableId.push(card.yieldPressureTableId);
    m_tensileModulusTableId.push(card.tensileModulusTableId);
    m_transverseShearModulusTableId.push(card.transverseShearModulusTableId);
    m_initialGapTableId.push(card.initialGapTableId);
    return row;
}

GasketMatTemperatureCard GasketMatTemperatureTable::card(std::uint32_t row) const
{
    GasketMatTemperatureCard card;
    card.id = m_id.get(row);
    card.membraneYoungsModulusTableId = m_membraneYoungsModulusTableId.get(row);
    card.membranePoissonRatioTableId = m_membranePoissonRatioTableId.get(row);
    card.membraneDensityTableId = m_membraneDensityTableId.get(row);
    card.loadingTableId = m_loadingTableId.get(row);
    card.unloadingTableIds = m_unloadingTableIds.get(row);
    card.yieldPressureTableId = m_yieldPressureTableId.get(row);
    card.tensileModulusTableId = m_tensileModulusTableId.get(row);
    card.transverseShearModulusTableId = m_transverseShearModulusTableId.get(row);
    card.initialGapTableId = m_initialGapTableId.get(row);
    return card;
}

void GasketMatTemperatureTable::store(std::uint32_t row, const GasketMatTemperatureCard& card)
{
    m_id.set(row, card.id);
    m_membraneYoungsModulusTableId.set(row, card.membraneYoungsModulusTableId);
    m_membranePoissonRatioTableId.set(row, card.membranePoissonRatioTableId);
    m_membraneDensityTableId.set(row, card.membraneDensityTableId);
    m_loadingTableId.set(row, card.loadingTableId);
    m_unloadingTableIds.set(row, card.unloadingTableIds);
    m_yieldPressureTableId.set(row, card.yieldPressureTableId);
    m_tensileModulusTableId.set(row, card.tensileModulusTableId);
    m_transverseShearModulusTableId.set(row, card.transverseShearModulusTableId);
    m_initialGapTableId.set(row, card.initialGapTableId);
}

void GasketMatTemperatureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_membraneYoungsModulusTableId.keepRows(rows);
    m_membranePoissonRatioTableId.keepRows(rows);
    m_membraneDensityTableId.keepRows(rows);
    m_loadingTableId.keepRows(rows);
    m_unloadingTableIds.keepRows(rows);
    m_yieldPressureTableId.keepRows(rows);
    m_tensileModulusTableId.keepRows(rows);
    m_transverseShearModulusTableId.keepRows(rows);
    m_initialGapTableId.keepRows(rows);
}

void GasketMatTemperatureTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The ID is the one of the MATG that the card extends; the tables are TABLEMi cards.
    m_id.set(row, visitor.visit("id", ReferenceTarget::Material, m_id.get(row)));
    const auto visitTable = [&](Column<base::EntityId>& column, const char* field)
    {
        if (const base::EntityId id = column.get(row); id != 0)
            column.set(row, visitor.visit(field, ReferenceTarget::MaterialTable, id));
    };
    visitTable(m_membraneYoungsModulusTableId, "membraneYoungsModulusTableId");
    visitTable(m_membranePoissonRatioTableId, "membranePoissonRatioTableId");
    visitTable(m_membraneDensityTableId, "membraneDensityTableId");
    visitTable(m_loadingTableId, "loadingTableId");
    std::array<base::EntityId, GasketMatTemperatureCard::UNLOADING_TABLES> unloading = m_unloadingTableIds.get(row);
    const std::array<base::EntityId, GasketMatTemperatureCard::UNLOADING_TABLES> before = unloading;
    for (std::size_t index = 0; index < unloading.size(); ++index)
    {
        if (unloading[index] != 0)
            unloading[index] = visitor.visit(UNLOADING_NAMES[index], ReferenceTarget::MaterialTable, unloading[index]);
    }
    if (unloading != before)
        m_unloadingTableIds.set(row, unloading);
    visitTable(m_yieldPressureTableId, "yieldPressureTableId");
    visitTable(m_tensileModulusTableId, "tensileModulusTableId");
    visitTable(m_transverseShearModulusTableId, "transverseShearModulusTableId");
    visitTable(m_initialGapTableId, "initialGapTableId");
}

} // namespace oor::model
