#include "oor/model/cards/materials/userMatTable/UserMatTableTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int FIELD_DENSITY_TABLE = 4;
constexpr int FIELD_THERMAL_EXPANSION_TABLE = 5; // T(A1) to T(A3): fields 5 to 7
constexpr int FIELD_DAMPING_TABLE = 9;
constexpr int FIELD_TENSION_ALLOWABLE_TABLE = 10;
constexpr int FIELD_COMPRESSION_ALLOWABLE_TABLE = 11;
constexpr int FIELD_SHEAR_ALLOWABLE_TABLE = 12;

constexpr const char* THERMAL_EXPANSION_TABLE_NAMES[3] = {"thermalExpansion1TableId", "thermalExpansion2TableId",
                                                          "thermalExpansion3TableId"};

} // namespace

std::optional<UserMatTableCard> UserMatTableFormat::read(FieldReader& reader)
{
    UserMatTableCard card;
    card.id = reader.requiredInteger(1, "id");
    card.densityTableId = reader.integer(FIELD_DENSITY_TABLE, "densityTableId", 0);
    for (std::size_t index = 0; index < card.thermalExpansionTableIds.size(); ++index)
        card.thermalExpansionTableIds[index] = reader.integer(FIELD_THERMAL_EXPANSION_TABLE + static_cast<int>(index),
                                                              THERMAL_EXPANSION_TABLE_NAMES[index], 0);
    card.structuralDampingTableId = reader.integer(FIELD_DAMPING_TABLE, "structuralDampingTableId", 0);
    card.tensionAllowableTableId = reader.integer(FIELD_TENSION_ALLOWABLE_TABLE, "tensionAllowableTableId", 0);
    card.compressionAllowableTableId =
        reader.integer(FIELD_COMPRESSION_ALLOWABLE_TABLE, "compressionAllowableTableId", 0);
    card.shearAllowableTableId = reader.integer(FIELD_SHEAR_ALLOWABLE_TABLE, "shearAllowableTableId", 0);
    requireBlankFields(reader, {2, 3, 8}, FIELD_SHEAR_ALLOWABLE_TABLE);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> UserMatTableFormat::check(const UserMatTableCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (const auto problem = checkTableReference(card.densityTableId, "densityTableId"))
        return problem;
    for (std::size_t index = 0; index < card.thermalExpansionTableIds.size(); ++index)
    {
        if (const auto problem =
                checkTableReference(card.thermalExpansionTableIds[index], THERMAL_EXPANSION_TABLE_NAMES[index]))
            return problem;
    }
    if (const auto problem = checkTableReference(card.structuralDampingTableId, "structuralDampingTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.tensionAllowableTableId, "tensionAllowableTableId"))
        return problem;
    if (const auto problem = checkTableReference(card.compressionAllowableTableId, "compressionAllowableTableId"))
        return problem;
    return checkTableReference(card.shearAllowableTableId, "shearAllowableTableId");
}

void UserMatTableFormat::write(const UserMatTableCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(FIELD_DENSITY_TABLE, card.densityTableId, 0);
    for (std::size_t index = 0; index < card.thermalExpansionTableIds.size(); ++index)
        writer.integerUnlessDefault(FIELD_THERMAL_EXPANSION_TABLE + static_cast<int>(index),
                                    card.thermalExpansionTableIds[index], 0);
    writer.integerUnlessDefault(FIELD_DAMPING_TABLE, card.structuralDampingTableId, 0);
    writer.integerUnlessDefault(FIELD_TENSION_ALLOWABLE_TABLE, card.tensionAllowableTableId, 0);
    writer.integerUnlessDefault(FIELD_COMPRESSION_ALLOWABLE_TABLE, card.compressionAllowableTableId, 0);
    writer.integerUnlessDefault(FIELD_SHEAR_ALLOWABLE_TABLE, card.shearAllowableTableId, 0);
}

std::uint32_t UserMatTableTable::append(const UserMatTableCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void UserMatTableTable::store(std::uint32_t row, const UserMatTableCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void UserMatTableTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void UserMatTableTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void UserMatTableTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    UserMatTableCard& card = m_cards.edit(row);
    // The ID is the one of the MATUSR that the card extends; every table field names a TABLEM1 or a TABL3Di: the target
    // MaterialTable resolves to either (a TABLEMi first) and a renumbering of the TABLEMi or of the TABL3Di follows the
    // fields that name its own.
    card.id = visitor.visit("id", ReferenceTarget::Material, card.id);
    m_id.set(row, card.id);
    const auto visitTable = [&](base::EntityId& table, const char* field)
    {
        if (table != 0)
            table = visitor.visit(field, ReferenceTarget::MaterialTable, table);
    };
    visitTable(card.densityTableId, "densityTableId");
    for (std::size_t index = 0; index < card.thermalExpansionTableIds.size(); ++index)
        visitTable(card.thermalExpansionTableIds[index], THERMAL_EXPANSION_TABLE_NAMES[index]);
    visitTable(card.structuralDampingTableId, "structuralDampingTableId");
    visitTable(card.tensionAllowableTableId, "tensionAllowableTableId");
    visitTable(card.compressionAllowableTableId, "compressionAllowableTableId");
    visitTable(card.shearAllowableTableId, "shearAllowableTableId");
}

} // namespace oor::model
