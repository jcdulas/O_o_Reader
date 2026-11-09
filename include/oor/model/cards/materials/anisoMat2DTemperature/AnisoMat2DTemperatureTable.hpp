#pragma once

#include "oor/model/cards/materials/anisoMat2DTemperature/AnisoMat2DTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AnisoMat2DTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AnisoMat2DTemperatureCard& card);
    [[nodiscard]] static std::optional<AnisoMat2DTemperatureCard> read(FieldReader& reader);
    static void write(const AnisoMat2DTemperatureCard& card, FieldWriter& writer);
};

// All the MATT2 cards. The ID space is the card type's own: the ID is the one of the MAT2 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable). The groups of tables are (N, 6) and (N, 3) NumPy views.
class AnisoMat2DTemperatureTable final : public TypedTable<AnisoMat2DTemperatureTable, AnisoMat2DTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "AnisoMat2DTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = AnisoMat2DTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const AnisoMat2DTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const AnisoMat2DTemperatureCard& card, base::CardKey key);
    [[nodiscard]] AnisoMat2DTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AnisoMat2DTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 6>>& stiffnessTableIdsColumn() noexcept
    {
        return m_stiffnessTableIds;
    }
    [[nodiscard]] Column<base::EntityId>& densityTableIdColumn() noexcept { return m_densityTableId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& thermalExpansionTableIdsColumn() noexcept
    {
        return m_thermalExpansionTableIds;
    }
    [[nodiscard]] Column<base::EntityId>& structuralDampingTableIdColumn() noexcept
    {
        return m_structuralDampingTableId;
    }
    [[nodiscard]] Column<base::EntityId>& tensionAllowableTableIdColumn() noexcept { return m_tensionAllowableTableId; }
    [[nodiscard]] Column<base::EntityId>& compressionAllowableTableIdColumn() noexcept
    {
        return m_compressionAllowableTableId;
    }
    [[nodiscard]] Column<base::EntityId>& shearAllowableTableIdColumn() noexcept { return m_shearAllowableTableId; }
    [[nodiscard]] Column<std::array<base::EntityId, 6>>& structuralDampingTermTableIdsColumn() noexcept
    {
        return m_structuralDampingTermTableIds;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 6>> m_stiffnessTableIds;
    Column<base::EntityId> m_densityTableId;
    Column<std::array<base::EntityId, 3>> m_thermalExpansionTableIds;
    Column<base::EntityId> m_structuralDampingTableId;
    Column<base::EntityId> m_tensionAllowableTableId;
    Column<base::EntityId> m_compressionAllowableTableId;
    Column<base::EntityId> m_shearAllowableTableId;
    Column<std::array<base::EntityId, 6>> m_structuralDampingTermTableIds;
};

} // namespace oor::model
