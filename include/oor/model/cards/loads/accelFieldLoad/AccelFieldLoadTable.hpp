#pragma once

#include "oor/model/cards/loads/accelFieldLoad/AccelFieldLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AccelFieldLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AccelFieldLoadCard& card);
    [[nodiscard]] static std::optional<AccelFieldLoadCard> read(FieldReader& reader);
    static void write(const AccelFieldLoadCard& card, FieldWriter& writer);
};

// All the ACCEL cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load), as for the other
// loads. CID is a reference to a coordinate system. Every field is a column (a companion package reads the inertial
// loads as arrays): DIR is the index of its word in ACCEL_FIELD_DIRECTION_WORDS, and the (LOC, VAL) pairs of
// all the cards are one column of lists.
class AccelFieldLoadTable final : public TypedTable<AccelFieldLoadTable, AccelFieldLoadCard>
{
public:
    static constexpr std::string_view NAME = "AccelFieldLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AccelFieldLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AccelFieldLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AccelFieldLoadCard& card, base::CardKey key);
    [[nodiscard]] AccelFieldLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AccelFieldLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& directionXColumn() noexcept { return m_directionX; }
    [[nodiscard]] Column<double>& directionYColumn() noexcept { return m_directionY; }
    [[nodiscard]] Column<double>& directionZColumn() noexcept { return m_directionZ; }
    [[nodiscard]] Column<std::uint8_t>& directionColumn() noexcept { return m_direction; }
    [[nodiscard]] VariableLengthColumn<std::array<double, 2>>& pointsColumn() noexcept { return m_points; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_frameId;
    Column<double> m_directionX;
    Column<double> m_directionY;
    Column<double> m_directionZ;
    Column<std::uint8_t> m_direction;
    VariableLengthColumn<std::array<double, 2>> m_points;
};

} // namespace oor::model
