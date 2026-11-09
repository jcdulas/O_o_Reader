#pragma once

#include "oor/model/cards/materials/gasketMatTemperature/GasketMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct GasketMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GasketMatTemperatureCard& card);
    [[nodiscard]] static std::optional<GasketMatTemperatureCard> read(FieldReader& reader);
    static void write(const GasketMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATTG cards. The ID space is the card type's own: the ID is the one of the MATG that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable).
class GasketMatTemperatureTable final : public TypedTable<GasketMatTemperatureTable, GasketMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "GasketMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = GasketMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const GasketMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const GasketMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] GasketMatTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GasketMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& membraneYoungsModulusTableIdColumn() noexcept
    {
        return m_membraneYoungsModulusTableId;
    }
    [[nodiscard]] Column<base::EntityId>& membranePoissonRatioTableIdColumn() noexcept
    {
        return m_membranePoissonRatioTableId;
    }
    [[nodiscard]] Column<base::EntityId>& membraneDensityTableIdColumn() noexcept { return m_membraneDensityTableId; }
    [[nodiscard]] Column<base::EntityId>& loadingTableIdColumn() noexcept { return m_loadingTableId; }
    [[nodiscard]] Column<std::array<base::EntityId, GasketMatTemperatureCard::UNLOADING_TABLES>>&
    unloadingTableIdsColumn() noexcept
    {
        return m_unloadingTableIds;
    }
    [[nodiscard]] Column<base::EntityId>& yieldPressureTableIdColumn() noexcept { return m_yieldPressureTableId; }
    [[nodiscard]] Column<base::EntityId>& tensileModulusTableIdColumn() noexcept { return m_tensileModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& transverseShearModulusTableIdColumn() noexcept
    {
        return m_transverseShearModulusTableId;
    }
    [[nodiscard]] Column<base::EntityId>& initialGapTableIdColumn() noexcept { return m_initialGapTableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_membraneYoungsModulusTableId;
    Column<base::EntityId> m_membranePoissonRatioTableId;
    Column<base::EntityId> m_membraneDensityTableId;
    Column<base::EntityId> m_loadingTableId;
    Column<std::array<base::EntityId, GasketMatTemperatureCard::UNLOADING_TABLES>> m_unloadingTableIds;
    Column<base::EntityId> m_yieldPressureTableId;
    Column<base::EntityId> m_tensileModulusTableId;
    Column<base::EntityId> m_transverseShearModulusTableId;
    Column<base::EntityId> m_initialGapTableId;
};

} // namespace oor::model
