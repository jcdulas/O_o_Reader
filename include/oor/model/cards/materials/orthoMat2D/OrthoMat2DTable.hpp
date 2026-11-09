#pragma once

#include "oor/model/cards/materials/orthoMat2D/OrthoMat2DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <limits>
#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat2DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat2DCard& card);
    [[nodiscard]] static std::optional<OrthoMat2DCard> read(FieldReader& reader);
    static void write(const OrthoMat2DCard& card, FieldWriter& writer);
};

// All the MAT8 cards. The three groups of Hashin criteria are (N, 6) and (N, 9) NumPy views, all zero for a card that
// has no such group; `hashin` says which cards have which group (bit 1 HFAIL, bit 2 HTAPE, bit 4 HFABR). G1Z and G2Z
// are NaN where the card leaves them blank.
class OrthoMat2DTable final : public TypedTable<OrthoMat2DTable, OrthoMat2DCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat2D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = OrthoMat2DFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat2DCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat2DCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat2DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const OrthoMat2DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& youngsModulus1Column() noexcept { return m_youngsModulus1; }
    [[nodiscard]] Column<double>& youngsModulus2Column() noexcept { return m_youngsModulus2; }
    [[nodiscard]] Column<double>& poissonRatio12Column() noexcept { return m_poissonRatio12; }
    [[nodiscard]] Column<double>& shearModulus12Column() noexcept { return m_shearModulus12; }
    [[nodiscard]] Column<double>& shearModulus1ZColumn() noexcept { return m_shearModulus1Z; }
    [[nodiscard]] Column<double>& shearModulus2ZColumn() noexcept { return m_shearModulus2Z; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& thermalExpansion1Column() noexcept { return m_thermalExpansion1; }
    [[nodiscard]] Column<double>& thermalExpansion2Column() noexcept { return m_thermalExpansion2; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& tensionStrength1Column() noexcept { return m_tensionStrength1; }
    [[nodiscard]] Column<double>& compressionStrength1Column() noexcept { return m_compressionStrength1; }
    [[nodiscard]] Column<double>& tensionStrength2Column() noexcept { return m_tensionStrength2; }
    [[nodiscard]] Column<double>& compressionStrength2Column() noexcept { return m_compressionStrength2; }
    [[nodiscard]] Column<double>& shearStrengthColumn() noexcept { return m_shearStrength; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& interactionTerm12Column() noexcept { return m_interactionTerm12; }
    [[nodiscard]] Column<double>& strainAllowablesColumn() noexcept { return m_strainAllowables; }
    [[nodiscard]] Column<std::uint8_t>& hashinColumn() noexcept { return m_hashin; }
    [[nodiscard]] Column<std::array<double, 6>>& hashinUnidirectionalColumn() noexcept
    {
        return m_hashinUnidirectional;
    }
    [[nodiscard]] Column<std::array<double, 9>>& hashinTapeColumn() noexcept { return m_hashinTape; }
    [[nodiscard]] Column<std::array<double, 9>>& hashinFabricColumn() noexcept { return m_hashinFabric; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_youngsModulus1;
    Column<double> m_youngsModulus2;
    Column<double> m_poissonRatio12;
    Column<double> m_shearModulus12;
    static constexpr double BLANK = std::numeric_limits<double>::quiet_NaN();
    Column<double> m_shearModulus1Z{BLANK};
    Column<double> m_shearModulus2Z{BLANK};
    Column<double> m_density;
    Column<double> m_thermalExpansion1;
    Column<double> m_thermalExpansion2;
    Column<double> m_referenceTemperature;
    Column<double> m_tensionStrength1;
    Column<double> m_compressionStrength1;
    Column<double> m_tensionStrength2;
    Column<double> m_compressionStrength2;
    Column<double> m_shearStrength;
    Column<double> m_structuralDamping;
    Column<double> m_interactionTerm12;
    Column<double> m_strainAllowables;
    Column<std::uint8_t> m_hashin;
    Column<std::array<double, 6>> m_hashinUnidirectional;
    Column<std::array<double, 9>> m_hashinTape;
    Column<std::array<double, 9>> m_hashinFabric;
};

} // namespace oor::model
