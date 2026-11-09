#pragma once

#include "oor/model/cards/materials/axisymOrthoMatStressDependent/AxisymOrthoMatStressDependentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymOrthoMatStressDependentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymOrthoMatStressDependentCard& card);
    [[nodiscard]] static std::optional<AxisymOrthoMatStressDependentCard> read(FieldReader& reader);
    static void write(const AxisymOrthoMatStressDependentCard& card, FieldWriter& writer);
};

// All the MATS3 cards. The ID space is the card type's own: the ID is the one of the MAT3 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it). Every table field is a reference to a
// TABL3D0, TABL3D1 or TABL3D2 (ReferenceTarget::MultiDimensionalTable), so a renumbering of those tables follows into
// the card.
class AxisymOrthoMatStressDependentTable final
    : public TypedTable<AxisymOrthoMatStressDependentTable, AxisymOrthoMatStressDependentCard>
{
public:
    static constexpr std::string_view NAME = "AxisymOrthoMatStressDependent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = AxisymOrthoMatStressDependentFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymOrthoMatStressDependentCard& card) noexcept
    {
        return card.id;
    }
    std::uint32_t append(const AxisymOrthoMatStressDependentCard& card, base::CardKey key);
    [[nodiscard]] AxisymOrthoMatStressDependentCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymOrthoMatStressDependentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& youngsModulusXTableIdColumn() noexcept { return m_youngsModulusXTableId; }
    [[nodiscard]] Column<base::EntityId>& youngsModulusThetaTableIdColumn() noexcept
    {
        return m_youngsModulusThetaTableId;
    }
    [[nodiscard]] Column<base::EntityId>& youngsModulusZTableIdColumn() noexcept { return m_youngsModulusZTableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatioXthTableIdColumn() noexcept { return m_poissonRatioXthTableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatioThzTableIdColumn() noexcept { return m_poissonRatioThzTableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatioZxTableIdColumn() noexcept { return m_poissonRatioZxTableId; }
    [[nodiscard]] Column<base::EntityId>& densityTableIdColumn() noexcept { return m_densityTableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulusZxTableIdColumn() noexcept { return m_shearModulusZxTableId; }
    [[nodiscard]] Column<base::EntityId>& thermalExpansionXTableIdColumn() noexcept
    {
        return m_thermalExpansionXTableId;
    }
    [[nodiscard]] Column<base::EntityId>& thermalExpansionThetaTableIdColumn() noexcept
    {
        return m_thermalExpansionThetaTableId;
    }
    [[nodiscard]] Column<base::EntityId>& thermalExpansionZTableIdColumn() noexcept
    {
        return m_thermalExpansionZTableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_youngsModulusXTableId;
    Column<base::EntityId> m_youngsModulusThetaTableId;
    Column<base::EntityId> m_youngsModulusZTableId;
    Column<base::EntityId> m_poissonRatioXthTableId;
    Column<base::EntityId> m_poissonRatioThzTableId;
    Column<base::EntityId> m_poissonRatioZxTableId;
    Column<base::EntityId> m_densityTableId;
    Column<base::EntityId> m_shearModulusZxTableId;
    Column<base::EntityId> m_thermalExpansionXTableId;
    Column<base::EntityId> m_thermalExpansionThetaTableId;
    Column<base::EntityId> m_thermalExpansionZTableId;
};

} // namespace oor::model
