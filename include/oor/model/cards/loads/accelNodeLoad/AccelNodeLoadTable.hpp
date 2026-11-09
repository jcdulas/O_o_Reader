#pragma once

#include "oor/model/cards/loads/accelNodeLoad/AccelNodeLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AccelNodeLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AccelNodeLoadCard& card);
    [[nodiscard]] static std::optional<AccelNodeLoadCard> read(FieldReader& reader);
    static void write(const AccelNodeLoadCard& card, FieldWriter& writer);
};

// All the ACCEL1 cards. The ID is a load set ID shared by many cards (IdSpace::Group, category Load). CID is a
// reference to a coordinate system; a single grid point of the list is a Node reference and the ends of a THRU range
// are NodeInRange (the grid points between them need not exist). Every field is a column (a companion package reads the
// inertial loads as arrays): the grid lists of all the cards are one column of (first, last, by) runs.
class AccelNodeLoadTable final : public TypedTable<AccelNodeLoadTable, AccelNodeLoadCard>
{
public:
    static constexpr std::string_view NAME = "AccelNodeLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AccelNodeLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AccelNodeLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AccelNodeLoadCard& card, base::CardKey key);
    [[nodiscard]] AccelNodeLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AccelNodeLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }
    [[nodiscard]] Column<double>& magnitudeColumn() noexcept { return m_magnitude; }
    [[nodiscard]] Column<double>& directionXColumn() noexcept { return m_directionX; }
    [[nodiscard]] Column<double>& directionYColumn() noexcept { return m_directionY; }
    [[nodiscard]] Column<double>& directionZColumn() noexcept { return m_directionZ; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& gridsColumn() noexcept { return m_grids; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_frameId;
    Column<double> m_magnitude;
    Column<double> m_directionX;
    Column<double> m_directionY;
    Column<double> m_directionZ;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_grids;
};

} // namespace oor::model
