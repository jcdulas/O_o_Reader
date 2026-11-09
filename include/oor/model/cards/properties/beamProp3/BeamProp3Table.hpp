#pragma once

#include "oor/model/cards/properties/beamProp3/BeamProp3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BeamProp3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamProp3Card& card);
    [[nodiscard]] static std::optional<BeamProp3Card> read(FieldReader& reader);
    static void write(const BeamProp3Card& card, FieldWriter& writer);
};

// All the PBEAM3 cards. The values of the three stations are fixed-size arrays of (N, 3 x n) NumPy views without copy:
// the n values of end A, then those of B, then those of C.
class BeamProp3Table final : public TypedTable<BeamProp3Table, BeamProp3Card>
{
public:
    static constexpr std::string_view NAME = "BeamProp3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeamProp3Format;

    BeamProp3Table() : m_shearFactor({1.0, 1.0}) {}

    [[nodiscard]] static base::EntityId cardId(const BeamProp3Card& card) noexcept { return card.id; }
    std::uint32_t append(const BeamProp3Card& card, base::CardKey key);
    [[nodiscard]] BeamProp3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const BeamProp3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::array<double, 18>>& sectionColumn() noexcept { return m_section; }
    [[nodiscard]] Column<std::array<double, 24>>& stressPointsColumn() noexcept { return m_stressPoints; }
    [[nodiscard]] Column<std::array<std::uint8_t, 2>>& stressOutputColumn() noexcept { return m_stressOutput; }
    [[nodiscard]] Column<std::array<double, 2>>& shearFactorColumn() noexcept { return m_shearFactor; }
    [[nodiscard]] Column<std::array<double, 6>>& neutralAxisColumn() noexcept { return m_neutralAxis; }
    [[nodiscard]] Column<std::array<double, 6>>& nsmCenterColumn() noexcept { return m_nsmCenter; }
    [[nodiscard]] Column<std::array<double, 9>>& nsmInertiaColumn() noexcept { return m_nsmInertia; }
    [[nodiscard]] Column<std::array<double, 3>>& warpingCoefficientColumn() noexcept { return m_warpingCoefficient; }
    [[nodiscard]] Column<std::uint8_t>& gaussStressColumn() noexcept { return m_gaussStress; }
    [[nodiscard]] Column<std::uint8_t>& warpingGivenColumn() noexcept { return m_warpingGiven; }
    [[nodiscard]] Column<std::array<double, 36>>& warpingColumn() noexcept { return m_warping; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::array<double, 18>> m_section;           // A IZ IY IYZ J NSM of A, B, C
    Column<std::array<double, 24>> m_stressPoints;      // CY CZ DY DZ EY EZ FY FZ of A, B, C
    Column<std::array<std::uint8_t, 2>> m_stressOutput; // SO(B), SO(C): 1 for YES
    Column<std::array<double, 2>> m_shearFactor;        // KY, KZ
    Column<std::array<double, 6>> m_neutralAxis;        // NY NZ of A, B, C
    Column<std::array<double, 6>> m_nsmCenter;          // MY MZ of A, B, C
    Column<std::array<double, 9>> m_nsmInertia;         // NSIY NSIZ NSIYZ of A, B, C
    Column<std::array<double, 3>> m_warpingCoefficient; // CW of A, B, C
    Column<std::uint8_t> m_gaussStress;
    Column<std::uint8_t> m_warpingGiven;
    Column<std::array<double, 36>> m_warping; // the 12 values of A, then B, then C
};

} // namespace oor::model
