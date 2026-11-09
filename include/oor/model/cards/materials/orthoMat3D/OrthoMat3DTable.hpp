#pragma once

#include "oor/model/cards/materials/orthoMat3D/OrthoMat3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat3DCard& card);
    [[nodiscard]] static std::optional<OrthoMat3DCard> read(FieldReader& reader);
    static void write(const OrthoMat3DCard& card, FieldWriter& writer);
};

// All the MAT11 cards. The ID is a material ID, in the space every material type shares (IdSpace::Material).
class OrthoMat3DTable final : public TypedTable<OrthoMat3DTable, OrthoMat3DCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = OrthoMat3DFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat3DCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat3DCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const OrthoMat3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT11 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& youngsModulus1Column() noexcept { return m_youngsModulus1; }
    [[nodiscard]] Column<double>& youngsModulus2Column() noexcept { return m_youngsModulus2; }
    [[nodiscard]] Column<double>& youngsModulus3Column() noexcept { return m_youngsModulus3; }
    [[nodiscard]] Column<double>& poissonRatio12Column() noexcept { return m_poissonRatio12; }
    [[nodiscard]] Column<double>& poissonRatio13Column() noexcept { return m_poissonRatio13; }
    [[nodiscard]] Column<double>& poissonRatio23Column() noexcept { return m_poissonRatio23; }
    [[nodiscard]] Column<double>& shearModulus12Column() noexcept { return m_shearModulus12; }
    [[nodiscard]] Column<double>& shearModulus13Column() noexcept { return m_shearModulus13; }
    [[nodiscard]] Column<double>& shearModulus23Column() noexcept { return m_shearModulus23; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& thermalExpansion1Column() noexcept { return m_thermalExpansion1; }
    [[nodiscard]] Column<double>& thermalExpansion2Column() noexcept { return m_thermalExpansion2; }
    [[nodiscard]] Column<double>& thermalExpansion3Column() noexcept { return m_thermalExpansion3; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_youngsModulus1;
    Column<double> m_youngsModulus2;
    Column<double> m_youngsModulus3;
    Column<double> m_poissonRatio12;
    Column<double> m_poissonRatio13;
    Column<double> m_poissonRatio23;
    Column<double> m_shearModulus12;
    Column<double> m_shearModulus13;
    Column<double> m_shearModulus23;
    Column<double> m_density;
    Column<double> m_thermalExpansion1;
    Column<double> m_thermalExpansion2;
    Column<double> m_thermalExpansion3;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
};

} // namespace oor::model
