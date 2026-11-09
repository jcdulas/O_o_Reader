#pragma once

#include "oor/model/cards/loads/rotationalForceLoad/RotationalForceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RotationalForceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotationalForceLoadCard& card);
    [[nodiscard]] static std::optional<RotationalForceLoadCard> read(FieldReader& reader);
    static void write(const RotationalForceLoadCard& card, FieldWriter& writer);
};

// All the RFORCE cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class RotationalForceLoadTable final : public TypedTable<RotationalForceLoadTable, RotationalForceLoadCard>
{
public:
    static constexpr std::string_view NAME = "RotationalForceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = RotationalForceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const RotationalForceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotationalForceLoadCard& card, base::CardKey key);
    [[nodiscard]] RotationalForceLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RotationalForceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& magnitudeColumn() noexcept { return m_magnitude; }
    [[nodiscard]] Column<std::array<double, 3>>& axisColumn() noexcept { return m_axis; }
    [[nodiscard]] Column<std::int32_t>& methodNameColumn() noexcept { return m_methodName; }
    [[nodiscard]] Column<double>& angularAccelerationColumn() noexcept { return m_angularAcceleration; }
    [[nodiscard]] Column<std::int32_t>& mainBulkFlagColumn() noexcept { return m_mainBulkFlag; }
    [[nodiscard]] Column<base::EntityId>& regionIdColumn() noexcept { return m_regionId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<base::EntityId> m_frameId;
    Column<double> m_magnitude;
    Column<std::array<double, 3>> m_axis;
    Column<std::int32_t> m_methodName;
    Column<double> m_angularAcceleration;
    Column<std::int32_t> m_mainBulkFlag;
    Column<base::EntityId> m_regionId;
};

} // namespace oor::model
