#pragma once

#include "oor/model/cards/thermal/fluidVirtualMass/FluidVirtualMassCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct FluidVirtualMassFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidVirtualMassCard& card);
    [[nodiscard]] static std::optional<FluidVirtualMassCard> read(FieldReader& reader);
    static void write(const FluidVirtualMassCard& card, FieldWriter& writer);
};

// All the MFLUID cards. The ID is a set ID shared by many cards (IdSpace::Group, category Other: the MFLUID Case
// Control command selects it). CID is a reference to a coordinate system. An absent ZFS is NaN.
class FluidVirtualMassTable final : public TypedTable<FluidVirtualMassTable, FluidVirtualMassCard>
{
public:
    static constexpr std::string_view NAME = "FluidVirtualMass";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = FluidVirtualMassFormat;

    FluidVirtualMassTable();

    [[nodiscard]] static base::EntityId cardId(const FluidVirtualMassCard& card) noexcept { return card.id; }
    std::uint32_t append(const FluidVirtualMassCard& card, base::CardKey key);
    [[nodiscard]] FluidVirtualMassCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FluidVirtualMassCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& freeSurfaceZColumn() noexcept { return m_freeSurfaceZ; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<base::EntityId>& oneSideElemListIdColumn() noexcept { return m_oneSideElemListId; }
    [[nodiscard]] Column<base::EntityId>& bothSidesElemListIdColumn() noexcept { return m_bothSidesElemListId; }
    [[nodiscard]] Column<std::string>& planeSymmetry1Column() noexcept { return m_planeSymmetry1; }
    [[nodiscard]] Column<std::string>& planeSymmetry2Column() noexcept { return m_planeSymmetry2; }
    [[nodiscard]] Column<double>& characteristicLengthColumn() noexcept { return m_characteristicLength; }
    [[nodiscard]] Column<double>& exactIntegrationRatioColumn() noexcept { return m_exactIntegrationRatio; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_frameId;
    Column<double> m_freeSurfaceZ;
    Column<double> m_density;
    Column<base::EntityId> m_oneSideElemListId;
    Column<base::EntityId> m_bothSidesElemListId;
    Column<std::string> m_planeSymmetry1;
    Column<std::string> m_planeSymmetry2;
    Column<double> m_characteristicLength;
    Column<double> m_exactIntegrationRatio;
};

} // namespace oor::model
