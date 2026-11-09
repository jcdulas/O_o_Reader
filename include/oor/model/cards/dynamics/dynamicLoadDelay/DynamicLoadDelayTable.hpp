#pragma once

#include "oor/model/cards/dynamics/dynamicLoadDelay/DynamicLoadDelayCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct DynamicLoadDelayFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DynamicLoadDelayCard& card);
    [[nodiscard]] static std::optional<DynamicLoadDelayCard> read(FieldReader& reader);
    static void write(const DynamicLoadDelayCard& card, FieldWriter& writer);
};

// All the DELAY cards. The ID is a set ID shared by many cards (IdSpace::Group, category Delay), which the DELAY field
// of a dynamic load refers to. The two groups of a card are kept as three columns of two values.
class DynamicLoadDelayTable final : public TypedTable<DynamicLoadDelayTable, DynamicLoadDelayCard>
{
public:
    static constexpr std::string_view NAME = "DynamicLoadDelay";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Delay, base::IdSpace::Group};
    using Format = DynamicLoadDelayFormat;

    [[nodiscard]] static base::EntityId cardId(const DynamicLoadDelayCard& card) noexcept { return card.id; }
    std::uint32_t append(const DynamicLoadDelayCard& card, base::CardKey key);
    [[nodiscard]] DynamicLoadDelayCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DynamicLoadDelayCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& pointsColumn() noexcept { return m_points; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::array<double, 2>>& delaysColumn() noexcept { return m_delays; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_points;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<std::array<double, 2>> m_delays;
};

} // namespace oor::model
