#pragma once

#include "oor/model/cards/dynamics/dynamicLoadPhase/DynamicLoadPhaseCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct DynamicLoadPhaseFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DynamicLoadPhaseCard& card);
    [[nodiscard]] static std::optional<DynamicLoadPhaseCard> read(FieldReader& reader);
    static void write(const DynamicLoadPhaseCard& card, FieldWriter& writer);
};

// All the DPHASE cards. The ID is a set ID shared by many cards (IdSpace::Group, category PhaseLead), which the DPHASE
// field of a dynamic load refers to. The two groups of a card are kept as three columns of two values.
class DynamicLoadPhaseTable final : public TypedTable<DynamicLoadPhaseTable, DynamicLoadPhaseCard>
{
public:
    static constexpr std::string_view NAME = "DynamicLoadPhase";
    static inline const CardTypeInfo INFO{NAME, CardCategory::PhaseLead, base::IdSpace::Group};
    using Format = DynamicLoadPhaseFormat;

    [[nodiscard]] static base::EntityId cardId(const DynamicLoadPhaseCard& card) noexcept { return card.id; }
    std::uint32_t append(const DynamicLoadPhaseCard& card, base::CardKey key);
    [[nodiscard]] DynamicLoadPhaseCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DynamicLoadPhaseCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& pointsColumn() noexcept { return m_points; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<std::array<double, 2>>& phaseAnglesColumn() noexcept { return m_phaseAngles; }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, 2>> m_points;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<std::array<double, 2>> m_phaseAngles;
};

} // namespace oor::model
