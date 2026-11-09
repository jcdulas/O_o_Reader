#pragma once

#include "oor/model/cards/materials/fluidMat/FluidMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FluidMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidMatCard& card);
    [[nodiscard]] static std::optional<FluidMatCard> read(FieldReader& reader);
    static void write(const FluidMatCard& card, FieldWriter& writer);

    // Fills BULK, RHO and C of `card` from the ones given: the missing one comes from the other two (C^2 = BULK / RHO).
    // Gives the problem when fewer than two are given or when the ratio BULK / RHO has no square root.
    [[nodiscard]] static std::optional<CardProblem> resolve(std::optional<double> bulkModulus,
                                                            std::optional<double> density,
                                                            std::optional<double> soundSpeed, FluidMatCard& card);
};

// All the MAT10 cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). ALPHA is
// NaN for a card that has none.
class FluidMatTable final : public TypedTable<FluidMatTable, FluidMatCard>
{
public:
    static constexpr std::string_view NAME = "FluidMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = FluidMatFormat;

    FluidMatTable();

    [[nodiscard]] static base::EntityId cardId(const FluidMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const FluidMatCard& card, base::CardKey key);
    [[nodiscard]] FluidMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FluidMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT10 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& bulkModulusColumn() noexcept { return m_bulkModulus; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& soundSpeedColumn() noexcept { return m_soundSpeed; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& normalizedAdmittanceColumn() noexcept { return m_normalizedAdmittance; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_bulkModulus;
    Column<double> m_density;
    Column<double> m_soundSpeed;
    Column<double> m_structuralDamping;
    Column<double> m_normalizedAdmittance;
};

} // namespace oor::model
