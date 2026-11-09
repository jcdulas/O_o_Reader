#pragma once

#include "oor/model/cards/materials/anisoMat3D/AnisoMat3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AnisoMat3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AnisoMat3DCard& card);
    [[nodiscard]] static std::optional<AnisoMat3DCard> read(FieldReader& reader);
    static void write(const AnisoMat3DCard& card, FieldWriter& writer);
};

// All the MAT9 cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The
// matrices are (N, 21) NumPy views; `hasStructuralDampingTerms` says which cards have a structural damping matrix (the
// others hold zeros).
class AnisoMat3DTable final : public TypedTable<AnisoMat3DTable, AnisoMat3DCard>
{
public:
    static constexpr std::string_view NAME = "AnisoMat3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = AnisoMat3DFormat;

    [[nodiscard]] static base::EntityId cardId(const AnisoMat3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const AnisoMat3DCard& card, base::CardKey key);
    [[nodiscard]] AnisoMat3DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AnisoMat3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT9 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<double, 21>>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<std::array<double, 6>>& thermalExpansionColumn() noexcept { return m_thermalExpansion; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<std::uint8_t>& hasStructuralDampingTermsColumn() noexcept
    {
        return m_hasStructuralDampingTerms;
    }
    [[nodiscard]] Column<std::array<double, 21>>& structuralDampingTermsColumn() noexcept
    {
        return m_structuralDampingTerms;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<double, 21>> m_stiffness;
    Column<double> m_density;
    Column<std::array<double, 6>> m_thermalExpansion;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<std::uint8_t> m_hasStructuralDampingTerms;
    Column<std::array<double, 21>> m_structuralDampingTerms;
};

} // namespace oor::model
