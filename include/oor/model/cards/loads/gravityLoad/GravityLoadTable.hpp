#pragma once

#include "oor/model/cards/loads/gravityLoad/GravityLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct GravityLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const GravityLoadCard& card);
    [[nodiscard]] static std::optional<GravityLoadCard> read(FieldReader& reader);
    static void write(const GravityLoadCard& card, FieldWriter& writer);
};

// All the GRAV cards. The ID is a load set ID shared by many cards (IdSpace::Group).
class GravityLoadTable final : public TypedTable<GravityLoadTable, GravityLoadCard>
{
public:
    static constexpr std::string_view NAME = "GravityLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = GravityLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const GravityLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const GravityLoadCard& card, base::CardKey key);
    [[nodiscard]] GravityLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const GravityLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& magnitudeColumn() noexcept { return m_magnitude; }
    [[nodiscard]] Column<std::array<double, 3>>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] Column<std::int32_t>& mainBulkFlagColumn() noexcept { return m_mainBulkFlag; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_frameId;
    Column<double> m_magnitude;
    Column<std::array<double, 3>> m_direction;
    Column<std::int32_t> m_mainBulkFlag;
};

} // namespace oor::model
