#include "oor/model/cards/materials/gasketMat/GasketMatTable.hpp"

#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The unloading tables are the fields 5 to 8 of the first line and 9 to 14 of the second.
constexpr int FIRST_UNLOADING_FIELD = 5;

int unloadingField(std::size_t index)
{
    return FIRST_UNLOADING_FIELD + static_cast<int>(index);
}

// The name of the code of each unloading path field.
constexpr const char* UNLOADING_NAMES[GasketMatCard::UNLOADING_TABLES] = {
    "unloadingTableId1", "unloadingTableId2", "unloadingTableId3", "unloadingTableId4", "unloadingTableId5",
    "unloadingTableId6", "unloadingTableId7", "unloadingTableId8", "unloadingTableId9", "unloadingTableId10"};

} // namespace

std::optional<GasketMatCard> GasketMatFormat::read(FieldReader& reader)
{
    GasketMatCard card;
    card.id = reader.requiredInteger(1, "id");
    card.membraneMatId = reader.requiredInteger(2, "membraneMatId");
    card.behaviorKind = reader.integer(3, "behaviorKind", 0);
    card.loadingTableId = reader.requiredInteger(4, "loadingTableId");
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
        card.unloadingTableIds[index] = reader.integer(unloadingField(index), UNLOADING_NAMES[index], 0);
    card.yieldPressure = reader.requiredReal(15, "yieldPressure");
    card.tensileModulus = reader.requiredReal(16, "tensileModulus");
    card.transverseShearModulus = reader.requiredReal(17, "transverseShearModulus");
    card.initialGap = reader.real(18, "initialGap", 0.0);
    card.yieldPressureTableId = reader.integer(19, "yieldPressureTableId", 0);
    card.tensileModulusTableId = reader.integer(20, "tensileModulusTableId", 0);
    card.transverseShearModulusTableId = reader.integer(21, "transverseShearModulusTableId", 0);
    card.initialGapTableId = reader.integer(22, "initialGapTableId", 0);
    requireBlankFields(reader, {}, 22);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> GasketMatFormat::check(const GasketMatCard& card)
{
    if (const auto problem = checkMaterialId(card.id))
        return problem;
    if (card.membraneMatId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of an isotropic material",
                           "membraneMatId"};
    if (card.behaviorKind != 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is 0: no other type is supported",
                           "behaviorKind"};
    if (card.loadingTableId <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be the ID of a stress table",
                           "loadingTableId"};
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
    {
        if (card.unloadingTableIds[index] < 0)
            return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative",
                               UNLOADING_NAMES[index]};
    }
    // YPRS, EPL and GPL are not zero.
    if (card.yieldPressure == 0.0 || !std::isfinite(card.yieldPressure))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "yieldPressure"};
    if (card.tensileModulus == 0.0 || !std::isfinite(card.tensileModulus))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "tensileModulus"};
    if (card.transverseShearModulus == 0.0 || !std::isfinite(card.transverseShearModulus))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero",
                           "transverseShearModulus"};
    if (!std::isfinite(card.initialGap))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number",
                           "initialGap"};
    for (const base::EntityId table : {card.yieldPressureTableId, card.tensileModulusTableId,
                                       card.transverseShearModulusTableId, card.initialGapTableId})
    {
        if (table < 0)
            return CardProblem{"card.bad-id", "a table ID must not be negative", "yieldPressureTableId"};
    }
    return std::nullopt;
}

void GasketMatFormat::write(const GasketMatCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.membraneMatId);
    writer.integerUnlessDefault(3, card.behaviorKind, 0);
    writer.integer(4, card.loadingTableId);
    for (std::size_t index = 0; index < card.unloadingTableIds.size(); ++index)
        writer.integerUnlessDefault(unloadingField(index), card.unloadingTableIds[index], 0);
    writer.real(15, card.yieldPressure);
    writer.real(16, card.tensileModulus);
    writer.real(17, card.transverseShearModulus);
    writer.realUnlessDefault(18, card.initialGap, 0.0);
    writer.integerUnlessDefault(19, card.yieldPressureTableId, 0);
    writer.integerUnlessDefault(20, card.tensileModulusTableId, 0);
    writer.integerUnlessDefault(21, card.transverseShearModulusTableId, 0);
    writer.integerUnlessDefault(22, card.initialGapTableId, 0);
}

std::uint32_t GasketMatTable::append(const GasketMatCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_membraneMatId.push(card.membraneMatId);
    m_behaviorKind.push(card.behaviorKind);
    m_loadingTableId.push(card.loadingTableId);
    m_unloadingTableIds.push(card.unloadingTableIds);
    m_yieldPressure.push(card.yieldPressure);
    m_tensileModulus.push(card.tensileModulus);
    m_transverseShearModulus.push(card.transverseShearModulus);
    m_initialGap.push(card.initialGap);
    m_yieldPressureTableId.push(card.yieldPressureTableId);
    m_tensileModulusTableId.push(card.tensileModulusTableId);
    m_transverseShearModulusTableId.push(card.transverseShearModulusTableId);
    m_initialGapTableId.push(card.initialGapTableId);
    return row;
}

GasketMatCard GasketMatTable::card(std::uint32_t row) const
{
    GasketMatCard card;
    card.id = m_id.get(row);
    card.membraneMatId = m_membraneMatId.get(row);
    card.behaviorKind = m_behaviorKind.get(row);
    card.loadingTableId = m_loadingTableId.get(row);
    card.unloadingTableIds = m_unloadingTableIds.get(row);
    card.yieldPressure = m_yieldPressure.get(row);
    card.tensileModulus = m_tensileModulus.get(row);
    card.transverseShearModulus = m_transverseShearModulus.get(row);
    card.initialGap = m_initialGap.get(row);
    card.yieldPressureTableId = m_yieldPressureTableId.get(row);
    card.tensileModulusTableId = m_tensileModulusTableId.get(row);
    card.transverseShearModulusTableId = m_transverseShearModulusTableId.get(row);
    card.initialGapTableId = m_initialGapTableId.get(row);
    return card;
}

void GasketMatTable::store(std::uint32_t row, const GasketMatCard& card)
{
    m_id.set(row, card.id);
    m_membraneMatId.set(row, card.membraneMatId);
    m_behaviorKind.set(row, card.behaviorKind);
    m_loadingTableId.set(row, card.loadingTableId);
    m_unloadingTableIds.set(row, card.unloadingTableIds);
    m_yieldPressure.set(row, card.yieldPressure);
    m_tensileModulus.set(row, card.tensileModulus);
    m_transverseShearModulus.set(row, card.transverseShearModulus);
    m_initialGap.set(row, card.initialGap);
    m_yieldPressureTableId.set(row, card.yieldPressureTableId);
    m_tensileModulusTableId.set(row, card.tensileModulusTableId);
    m_transverseShearModulusTableId.set(row, card.transverseShearModulusTableId);
    m_initialGapTableId.set(row, card.initialGapTableId);
}

void GasketMatTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_membraneMatId.keepRows(rows);
    m_behaviorKind.keepRows(rows);
    m_loadingTableId.keepRows(rows);
    m_unloadingTableIds.keepRows(rows);
    m_yieldPressure.keepRows(rows);
    m_tensileModulus.keepRows(rows);
    m_transverseShearModulus.keepRows(rows);
    m_initialGap.keepRows(rows);
    m_yieldPressureTableId.keepRows(rows);
    m_tensileModulusTableId.keepRows(rows);
    m_transverseShearModulusTableId.keepRows(rows);
    m_initialGapTableId.keepRows(rows);
}

void GasketMatTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    m_membraneMatId.set(row, visitor.visit("membraneMatId", ReferenceTarget::Material, m_membraneMatId.get(row)));
    m_loadingTableId.set(row, visitor.visit("loadingTableId", ReferenceTarget::StressTable, m_loadingTableId.get(row)));
    std::array<base::EntityId, GasketMatCard::UNLOADING_TABLES> unloading = m_unloadingTableIds.get(row);
    const std::array<base::EntityId, GasketMatCard::UNLOADING_TABLES> before = unloading;
    for (std::size_t index = 0; index < unloading.size(); ++index)
    {
        if (unloading[index] != 0)
            unloading[index] = visitor.visit(UNLOADING_NAMES[index], ReferenceTarget::StressTable, unloading[index]);
    }
    if (unloading != before)
        m_unloadingTableIds.set(row, unloading);
}

} // namespace oor::model
