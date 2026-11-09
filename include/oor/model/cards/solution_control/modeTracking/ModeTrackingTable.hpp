#pragma once

#include "oor/model/cards/solution_control/modeTracking/ModeTrackingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModeTrackingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModeTrackingCard& card);
    [[nodiscard]] static std::optional<ModeTrackingCard> read(FieldReader& reader);
    static void write(const ModeTrackingCard& card, FieldWriter& writer);
};

// All the MODTRAK cards. The ID is unique among the MODTRAK cards (IdSpace::PerCardType).
class ModeTrackingTable final : public TypedTable<ModeTrackingTable, ModeTrackingCard>
{
public:
    static constexpr std::string_view NAME = "ModeTracking";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModeTrackingFormat;

    [[nodiscard]] static base::EntityId cardId(const ModeTrackingCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModeTrackingCard& card, base::CardKey key);
    [[nodiscard]] ModeTrackingCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ModeTrackingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MODTRAK refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& lowRangeColumn() noexcept { return m_lowRange; }
    [[nodiscard]] Column<std::int32_t>& highRangeColumn() noexcept { return m_highRange; }
    [[nodiscard]] Column<double>& trackingFilterColumn() noexcept { return m_trackingFilter; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_lowRange;
    Column<std::int32_t> m_highRange;
    Column<double> m_trackingFilter{0.9};
};

} // namespace oor::model
