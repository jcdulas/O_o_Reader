#pragma once

#include "oor/model/cards/loads/axisymRingForceLoad/AxisymRingForceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymRingForceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymRingForceLoadCard& card);
    [[nodiscard]] static std::optional<AxisymRingForceLoadCard> read(FieldReader& reader);
    static void write(const AxisymRingForceLoadCard& card, FieldWriter& writer);
};

// All the FORCEAX cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class AxisymRingForceLoadTable final : public TypedTable<AxisymRingForceLoadTable, AxisymRingForceLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymRingForceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxisymRingForceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymRingForceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymRingForceLoadCard& card, base::CardKey key);
    [[nodiscard]] AxisymRingForceLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymRingForceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<std::int32_t>& harmonicIdColumn() noexcept { return m_harmonicId; }
    [[nodiscard]] Column<std::int32_t>& harmonicEndColumn() noexcept { return m_harmonicEnd; }
    [[nodiscard]] Column<double>& scaleFactorColumn() noexcept { return m_scaleFactor; }
    [[nodiscard]] Column<double>& radialForceColumn() noexcept { return m_radialForce; }
    [[nodiscard]] Column<double>& azimuthalForceColumn() noexcept { return m_azimuthalForce; }
    [[nodiscard]] Column<double>& axialForceColumn() noexcept { return m_axialForce; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<std::int32_t> m_harmonicId;
    Column<std::int32_t> m_harmonicEnd;
    Column<double> m_scaleFactor;
    Column<double> m_radialForce;
    Column<double> m_azimuthalForce;
    Column<double> m_axialForce;
};

} // namespace oor::model
