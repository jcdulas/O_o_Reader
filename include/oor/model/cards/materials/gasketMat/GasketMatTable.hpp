#pragma once

#include "oor/model/cards/materials/gasketMat/GasketMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasketMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasketMatCard& card);
    [[nodiscard]] static std::optional<GasketMatCard> read(FieldReader& reader);
    static void write(const GasketMatCard& card, FieldWriter& writer);
};

// All the MATG cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). IDMEM
// refers to a MAT1 (ReferenceTarget::Material) and the loading and unloading tables to TABLES1 cards (StressTable). The
// four tables that the program does not use yet (TABYPRS, TABEPL, TABGPL, TABGAP) are not visited.
class GasketMatTable final : public TypedTable<GasketMatTable, GasketMatCard>
{
public:
    static constexpr std::string_view NAME = "GasketMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = GasketMatFormat;

    [[nodiscard]] static base::EntityId cardId(const GasketMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasketMatCard& card, base::CardKey key);
    [[nodiscard]] GasketMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GasketMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& membraneMatIdColumn() noexcept { return m_membraneMatId; }
    [[nodiscard]] Column<std::int32_t>& behaviorKindColumn() noexcept { return m_behaviorKind; }
    [[nodiscard]] Column<base::EntityId>& loadingTableIdColumn() noexcept { return m_loadingTableId; }
    [[nodiscard]] Column<std::array<base::EntityId, GasketMatCard::UNLOADING_TABLES>>&
    unloadingTableIdsColumn() noexcept
    {
        return m_unloadingTableIds;
    }
    [[nodiscard]] Column<double>& yieldPressureColumn() noexcept { return m_yieldPressure; }
    [[nodiscard]] Column<double>& tensileModulusColumn() noexcept { return m_tensileModulus; }
    [[nodiscard]] Column<double>& transverseShearModulusColumn() noexcept { return m_transverseShearModulus; }
    [[nodiscard]] Column<double>& initialGapColumn() noexcept { return m_initialGap; }
    [[nodiscard]] Column<base::EntityId>& yieldPressureTableIdColumn() noexcept { return m_yieldPressureTableId; }
    [[nodiscard]] Column<base::EntityId>& tensileModulusTableIdColumn() noexcept { return m_tensileModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& transverseShearModulusTableIdColumn() noexcept
    {
        return m_transverseShearModulusTableId;
    }
    [[nodiscard]] Column<base::EntityId>& initialGapTableIdColumn() noexcept { return m_initialGapTableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_membraneMatId;
    Column<std::int32_t> m_behaviorKind;
    Column<base::EntityId> m_loadingTableId;
    Column<std::array<base::EntityId, GasketMatCard::UNLOADING_TABLES>> m_unloadingTableIds;
    Column<double> m_yieldPressure;
    Column<double> m_tensileModulus;
    Column<double> m_transverseShearModulus;
    Column<double> m_initialGap;
    Column<base::EntityId> m_yieldPressureTableId;
    Column<base::EntityId> m_tensileModulusTableId;
    Column<base::EntityId> m_transverseShearModulusTableId;
    Column<base::EntityId> m_initialGapTableId;
};

} // namespace oor::model
