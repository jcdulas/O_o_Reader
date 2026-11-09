#pragma once

#include "oor/model/cards/loads/axisymRingMomentLoad/AxisymRingMomentLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymRingMomentLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymRingMomentLoadCard& card);
    [[nodiscard]] static std::optional<AxisymRingMomentLoadCard> read(FieldReader& reader);
    static void write(const AxisymRingMomentLoadCard& card, FieldWriter& writer);
};

// All the MOMAX cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). RID names a
// RINGAX, a point of the node ID space (a node reference).
class AxisymRingMomentLoadTable final : public TypedTable<AxisymRingMomentLoadTable, AxisymRingMomentLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymRingMomentLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxisymRingMomentLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymRingMomentLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymRingMomentLoadCard& card, base::CardKey key);
    [[nodiscard]] AxisymRingMomentLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymRingMomentLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<std::int32_t>& harmonicIdColumn() noexcept { return m_harmonicId; }
    [[nodiscard]] Column<std::int32_t>& harmonicEndColumn() noexcept { return m_harmonicEnd; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<double>& radialMomentColumn() noexcept { return m_radialMoment; }
    [[nodiscard]] Column<double>& azimuthalMomentColumn() noexcept { return m_azimuthalMoment; }
    [[nodiscard]] Column<double>& axialMomentColumn() noexcept { return m_axialMoment; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<std::int32_t> m_harmonicId;
    Column<std::int32_t> m_harmonicEnd;
    Column<double> m_scaleFactor;
    Column<double> m_radialMoment;
    Column<double> m_azimuthalMoment;
    Column<double> m_axialMoment;
};

} // namespace oor::model
