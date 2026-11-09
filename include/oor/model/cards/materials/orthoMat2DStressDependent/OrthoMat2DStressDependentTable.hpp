#pragma once

#include "oor/model/cards/materials/orthoMat2DStressDependent/OrthoMat2DStressDependentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat2DStressDependentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat2DStressDependentCard& card);
    [[nodiscard]] static std::optional<OrthoMat2DStressDependentCard> read(FieldReader& reader);
    static void write(const OrthoMat2DStressDependentCard& card, FieldWriter& writer);
};

// All the MATS8 cards. The ID space is the card type's own: the ID is the one of the MAT8 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it). Every table field is a reference to a
// TABL3D0, TABL3D1 or TABL3D2 (ReferenceTarget::MultiDimensionalTable), so a renumbering of those tables follows into
// the card.
class OrthoMat2DStressDependentTable final
    : public TypedTable<OrthoMat2DStressDependentTable, OrthoMat2DStressDependentCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat2DStressDependent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = OrthoMat2DStressDependentFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat2DStressDependentCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat2DStressDependentCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat2DStressDependentCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const OrthoMat2DStressDependentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& youngsModulus1TableIdColumn() noexcept { return m_youngsModulus1TableId; }
    [[nodiscard]] Column<base::EntityId>& youngsModulus2TableIdColumn() noexcept { return m_youngsModulus2TableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatio12TableIdColumn() noexcept { return m_poissonRatio12TableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus12TableIdColumn() noexcept { return m_shearModulus12TableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus1ZTableIdColumn() noexcept { return m_shearModulus1ZTableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus2ZTableIdColumn() noexcept { return m_shearModulus2ZTableId; }
    [[nodiscard]] Column<base::EntityId>& densityTableIdColumn() noexcept { return m_densityTableId; }
    [[nodiscard]] Column<base::EntityId>& thermalExpansion1TableIdColumn() noexcept
    {
        return m_thermalExpansion1TableId;
    }
    [[nodiscard]] Column<base::EntityId>& thermalExpansion2TableIdColumn() noexcept
    {
        return m_thermalExpansion2TableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_youngsModulus1TableId;
    Column<base::EntityId> m_youngsModulus2TableId;
    Column<base::EntityId> m_poissonRatio12TableId;
    Column<base::EntityId> m_shearModulus12TableId;
    Column<base::EntityId> m_shearModulus1ZTableId;
    Column<base::EntityId> m_shearModulus2ZTableId;
    Column<base::EntityId> m_densityTableId;
    Column<base::EntityId> m_thermalExpansion1TableId;
    Column<base::EntityId> m_thermalExpansion2TableId;
};

} // namespace oor::model
