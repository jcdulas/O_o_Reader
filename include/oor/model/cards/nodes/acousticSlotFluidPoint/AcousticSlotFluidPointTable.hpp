#pragma once

#include "oor/model/cards/nodes/acousticSlotFluidPoint/AcousticSlotFluidPointCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSlotFluidPointFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSlotFluidPointCard& card);
    [[nodiscard]] static std::optional<AcousticSlotFluidPointCard> read(FieldReader& reader);
    static void write(const AcousticSlotFluidPointCard& card, FieldWriter& writer);
};

// All the GRIDF cards of a model: points of the node ID space (IdSpace::Node, category Point), kept in columns. A GRIDF
// names no other card; a GRIDS may name it (its IDF).
class AcousticSlotFluidPointTable final : public TypedTable<AcousticSlotFluidPointTable, AcousticSlotFluidPointCard>
{
public:
    static constexpr std::string_view NAME = "AcousticSlotFluidPoint";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Point, base::IdSpace::Node};
    using Format = AcousticSlotFluidPointFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticSlotFluidPointCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticSlotFluidPointCard& card, base::CardKey key);
    [[nodiscard]] AcousticSlotFluidPointCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticSlotFluidPointCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a GRIDF refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<double>& zCoordColumn() noexcept { return m_zCoord; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_radius;
    Column<double> m_zCoord;
};

} // namespace oor::model
