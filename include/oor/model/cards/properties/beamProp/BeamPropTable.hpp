#pragma once

#include "oor/model/cards/properties/beamProp/BeamPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BeamPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamPropCard& card);
    [[nodiscard]] static std::optional<BeamPropCard> read(FieldReader& reader);
    static void write(const BeamPropCard& card, FieldWriter& writer);

    // Gives a value to each section property (A, I1, I2, I12, J, NSM) that is NaN in a station after end A: at
    // X/XB = 1.0 the value of end A, in between the linear interpolation of the two ends. End A must have all its
    // values. Returns false when there are stations but none is at X/XB = 1.0.
    [[nodiscard]] static bool resolveBlankSections(std::vector<BeamPropStation>& stations);
};

// All the PBEAM cards. The stations of every card are four lists in CSR form (output flag, X/XB, section properties,
// stress points): the row of a card gives where its stations begin and how many there are. The pairs and quadruples
// of the footer are (N, 2) and (N, 4) NumPy views.
class BeamPropTable final : public TypedTable<BeamPropTable, BeamPropCard>
{
public:
    static constexpr std::string_view NAME = "BeamProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeamPropFormat;

    BeamPropTable() : m_shearFactor({1.0, 1.0}) {}

    [[nodiscard]] static base::EntityId cardId(const BeamPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeamPropCard& card, base::CardKey key);
    [[nodiscard]] BeamPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BeamPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] std::size_t stationCount(std::uint32_t row) const { return m_stationRatio.get(row).size(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] VariableLengthColumn<std::uint8_t>& stationOutputColumn() noexcept { return m_stationOutput; }
    [[nodiscard]] VariableLengthColumn<double>& stationRatioColumn() noexcept { return m_stationRatio; }
    // A, I1, I2, I12, J, NSM of each station.
    [[nodiscard]] VariableLengthColumn<std::array<double, 6>>& stationSectionColumn() noexcept
    {
        return m_stationSection;
    }
    // C1, C2, D1, D2, E1, E2, F1, F2 of each station.
    [[nodiscard]] VariableLengthColumn<std::array<double, 8>>& stationStressColumn() noexcept
    {
        return m_stationStress;
    }
    [[nodiscard]] Column<std::array<double, 2>>& shearFactorColumn() noexcept { return m_shearFactor; } // K1, K2
    [[nodiscard]] Column<std::array<double, 2>>& shearReliefColumn() noexcept { return m_shearRelief; } // S1, S2
    // NSI(A), NSI(B)
    [[nodiscard]] Column<std::array<double, 2>>& nsmInertiaColumn() noexcept { return m_nsmInertia; }
    // CW(A), CW(B)
    [[nodiscard]] Column<std::array<double, 2>>& warpingCoefficientColumn() noexcept { return m_warpingCoefficient; }
    // M1(A) M2(A) M1(B) M2(B)
    [[nodiscard]] Column<std::array<double, 4>>& nsmCenterColumn() noexcept { return m_nsmCenter; }
    // N1(A) N2(A) N1(B) N2(B)
    [[nodiscard]] Column<std::array<double, 4>>& neutralAxisColumn() noexcept { return m_neutralAxis; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    VariableLengthColumn<std::uint8_t> m_stationOutput;
    VariableLengthColumn<double> m_stationRatio;
    VariableLengthColumn<std::array<double, 6>> m_stationSection;
    VariableLengthColumn<std::array<double, 8>> m_stationStress;
    Column<std::array<double, 2>> m_shearFactor;
    Column<std::array<double, 2>> m_shearRelief;
    Column<std::array<double, 2>> m_nsmInertia;
    Column<std::array<double, 2>> m_warpingCoefficient;
    Column<std::array<double, 4>> m_nsmCenter;
    Column<std::array<double, 4>> m_neutralAxis;
};

} // namespace oor::model
