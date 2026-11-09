#pragma once

#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct BeamLumpedAreaPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BeamLumpedAreaPropCard& card);
    [[nodiscard]] static std::optional<BeamLumpedAreaPropCard> read(FieldReader& reader);
    static void write(const BeamLumpedAreaPropCard& card, FieldWriter& writer);
};

// All the PBCOMP cards. The lumped areas of every card are four lists (y, z, c, material) in CSR form: the row of a
// card gives where its areas begin and how many there are.
class BeamLumpedAreaPropTable final : public TypedTable<BeamLumpedAreaPropTable, BeamLumpedAreaPropCard>
{
public:
    static constexpr std::string_view NAME = "BeamLumpedAreaProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BeamLumpedAreaPropFormat;

    BeamLumpedAreaPropTable() : m_shearFactor({1.0, 1.0}) {}

    [[nodiscard]] static base::EntityId cardId(const BeamLumpedAreaPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BeamLumpedAreaPropCard& card, base::CardKey key);
    [[nodiscard]] BeamLumpedAreaPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BeamLumpedAreaPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] std::size_t areaCount(std::uint32_t row) const { return m_areaY.get(row).size(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::array<double, 6>>& sectionColumn() noexcept { return m_section; }
    [[nodiscard]] Column<std::array<double, 2>>& shearFactorColumn() noexcept { return m_shearFactor; }
    [[nodiscard]] Column<std::array<double, 2>>& nsmCenterColumn() noexcept { return m_nsmCenter; }
    [[nodiscard]] Column<std::array<double, 2>>& neutralAxisColumn() noexcept { return m_neutralAxis; }
    [[nodiscard]] Column<std::uint8_t>& symmetricOptionColumn() noexcept { return m_symmetricOption; }
    [[nodiscard]] VariableLengthColumn<double>& areaYColumn() noexcept { return m_areaY; }
    [[nodiscard]] VariableLengthColumn<double>& areaZColumn() noexcept { return m_areaZ; }
    [[nodiscard]] VariableLengthColumn<double>& areaFractionColumn() noexcept { return m_areaFraction; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& areaMatIdColumn() noexcept { return m_areaMatId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::array<double, 6>> m_section;     // A, I1, I2, I12, J, NSM
    Column<std::array<double, 2>> m_shearFactor; // K1, K2
    Column<std::array<double, 2>> m_nsmCenter;   // M1, M2
    Column<std::array<double, 2>> m_neutralAxis; // N1, N2
    Column<std::uint8_t> m_symmetricOption;
    VariableLengthColumn<double> m_areaY;
    VariableLengthColumn<double> m_areaZ;
    VariableLengthColumn<double> m_areaFraction;
    VariableLengthColumn<base::EntityId> m_areaMatId;
};

} // namespace oor::model
