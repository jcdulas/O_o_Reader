#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/nodes/acousticSlotSurfacePoint/AcousticSlotSurfacePointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSlotSurfacePointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSlotSurfacePointCard& card);
    [[nodiscard]] static std::optional<AcousticSlotSurfacePointCard> read(FieldReader& reader);
    static void write(const AcousticSlotSurfacePointCard& card, FieldWriter& writer);
};

// All the GRIDS cards of a model: points of the node ID space (IdSpace::Node, category Point), kept in columns. A blank
// W is a NaN in its column. The IDF names the GRIDF fluid point of an interface (a node reference).
class AcousticSlotSurfacePointTable final
    : public TypedTable<AcousticSlotSurfacePointTable, AcousticSlotSurfacePointCard>
{
public:
    static constexpr std::string_view NAME = "AcousticSlotSurfacePoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AcousticSlotSurfacePointFormat;

    AcousticSlotSurfacePointTable() : m_slotWidth(BLANK_REAL) {}

    [[nodiscard]] static base::EntityId cardId(const AcousticSlotSurfacePointCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticSlotSurfacePointCard& card, base::CardKey key);
    [[nodiscard]] AcousticSlotSurfacePointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticSlotSurfacePointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<double>& zCoordColumn() noexcept { return m_zCoord; }
    [[nodiscard]] Column<double>& slotWidthColumn() noexcept { return m_slotWidth; }
    [[nodiscard]] Column<base::EntityId>& fluidPointIdColumn() noexcept { return m_fluidPointId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_radius;
    Column<double> m_zCoord;
    Column<double> m_slotWidth;
    Column<base::EntityId> m_fluidPointId;
};

} // namespace oor::model
