#pragma once

#include "oor/model/cards/dynamics/rotorUnbalanceLoad/RotorUnbalanceLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct RotorUnbalanceLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RotorUnbalanceLoadCard& card);
    [[nodiscard]] static std::optional<RotorUnbalanceLoadCard> read(FieldReader& reader);
    static void write(const RotorUnbalanceLoadCard& card, FieldWriter& writer);
};

// All the UNBALNC cards. The ID is a set ID shared by the UNBALNC cards and the other dynamic loads (IdSpace::Group,
// category DynamicLoad: the DLOAD command selects it). GRID and the EPOINTs are points (Node); the integer MASS,
// ROFFSET and ZOFFSET are dynamic tables.
class RotorUnbalanceLoadTable final : public TypedTable<RotorUnbalanceLoadTable, RotorUnbalanceLoadCard>
{
public:
    static constexpr std::string_view NAME = "RotorUnbalanceLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DynamicLoad, base::IdSpace::Group};
    using Format = RotorUnbalanceLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const RotorUnbalanceLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const RotorUnbalanceLoadCard& card, base::CardKey key);
    [[nodiscard]] RotorUnbalanceLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RotorUnbalanceLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& massTableIdColumn() noexcept { return m_massTableId; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<base::EntityId>& nodeIdColumn() noexcept { return m_nodeId; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] Column<base::EntityId>& radialOffsetTableIdColumn() noexcept { return m_radialOffsetTableId; }
    [[nodiscard]] Column<double>& radialOffsetColumn() noexcept { return m_radialOffset; }
    [[nodiscard]] Column<double>& angularPositionColumn() noexcept { return m_angularPosition; }
    [[nodiscard]] Column<base::EntityId>& axialOffsetTableIdColumn() noexcept { return m_axialOffsetTableId; }
    [[nodiscard]] Column<double>& axialOffsetColumn() noexcept { return m_axialOffset; }
    [[nodiscard]] Column<double>& startTimeOrFrequencyColumn() noexcept { return m_startTimeOrFrequency; }
    [[nodiscard]] Column<double>& stopTimeOrFrequencyColumn() noexcept { return m_stopTimeOrFrequency; }
    [[nodiscard]] Column<std::uint8_t>& correctionFlagColumn() noexcept { return m_correctionFlag; }
    [[nodiscard]] Column<std::array<base::EntityId, RotorUnbalanceLoadCard::EXTRA_POINT_COUNT>>&
    extraPointsColumn() noexcept
    {
        return m_extraPoints;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_massTableId;
    Column<double> m_mass;
    Column<base::EntityId> m_nodeId;
    Column<std::array<double, 3>> m_direction;
    Column<base::EntityId> m_radialOffsetTableId;
    Column<double> m_radialOffset{1.0};
    Column<double> m_angularPosition;
    Column<base::EntityId> m_axialOffsetTableId;
    Column<double> m_axialOffset;
    Column<double> m_startTimeOrFrequency;
    Column<double> m_stopTimeOrFrequency{999999.0};
    Column<std::uint8_t> m_correctionFlag;
    Column<std::array<base::EntityId, RotorUnbalanceLoadCard::EXTRA_POINT_COUNT>> m_extraPoints;
};

} // namespace oor::model
