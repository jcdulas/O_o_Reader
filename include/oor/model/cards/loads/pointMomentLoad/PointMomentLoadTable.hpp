#pragma once

#include "oor/model/cards/loads/pointMomentLoad/PointMomentLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PointMomentLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PointMomentLoadCard& card);
    [[nodiscard]] static std::optional<PointMomentLoadCard> read(FieldReader& reader);
    static void write(const PointMomentLoadCard& card, FieldWriter& writer);
};

// All the MOMENT cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class PointMomentLoadTable final : public TypedTable<PointMomentLoadTable, PointMomentLoadCard>
{
public:
    static constexpr std::string_view NAME = "PointMomentLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = PointMomentLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const PointMomentLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const PointMomentLoadCard& card, base::CardKey key);
    [[nodiscard]] PointMomentLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PointMomentLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& magnitudeColumn() noexcept { return m_magnitude; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node;
    Column<base::EntityId> m_frameId;
    Column<double> m_magnitude;
    Column<std::array<double, 3>> m_direction;
};

} // namespace oor::model
