#pragma once

#include "oor/model/cards/materials/anisoMat2D/AnisoMat2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AnisoMat2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AnisoMat2DCard& card);
    [[nodiscard]] static std::optional<AnisoMat2DCard> read(FieldReader& reader);
    static void write(const AnisoMat2DCard& card, FieldWriter& writer);
};

// All the MAT2 cards. The two matrices are (N, 6) NumPy views and the expansion coefficients an (N, 3) view.
class AnisoMat2DTable final : public TypedTable<AnisoMat2DTable, AnisoMat2DCard>
{
public:
    static constexpr std::string_view NAME = "AnisoMat2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = AnisoMat2DFormat;

    [[nodiscard]] static base::EntityId cardId(const AnisoMat2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const AnisoMat2DCard& card, base::CardKey key);
    [[nodiscard]] AnisoMat2DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AnisoMat2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<double, 6>>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<std::array<double, 3>>& thermalExpansionColumn() noexcept { return m_thermalExpansion; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& tensionAllowableColumn() noexcept { return m_tensionAllowable; }
    [[nodiscard]] Column<double>& compressionAllowableColumn() noexcept { return m_compressionAllowable; }
    [[nodiscard]] Column<double>& shearAllowableColumn() noexcept { return m_shearAllowable; }
    [[nodiscard]] Column<base::EntityId>& stressFrameIdColumn() noexcept { return m_stressFrameId; }
    [[nodiscard]] Column<std::array<double, 6>>& structuralDampingTermsColumn() noexcept
    {
        return m_structuralDampingTerms;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<double, 6>> m_stiffness;
    Column<double> m_density;
    Column<std::array<double, 3>> m_thermalExpansion;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<double> m_tensionAllowable;
    Column<double> m_compressionAllowable;
    Column<double> m_shearAllowable;
    Column<base::EntityId> m_stressFrameId;
    Column<std::array<double, 6>> m_structuralDampingTerms;
};

} // namespace oor::model
