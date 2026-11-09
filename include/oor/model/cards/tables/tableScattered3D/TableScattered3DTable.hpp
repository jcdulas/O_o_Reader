#pragma once

#include "oor/model/cards/tables/tableScattered3D/TableScattered3DCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableScattered3DFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableScattered3DCard& card);
    [[nodiscard]] static std::optional<TableScattered3DCard> read(FieldReader& reader);
    static void write(const TableScattered3DCard& card, FieldWriter& writer);
};

// All the TABLE3D cards, in the category MultiDimensionalTable: the TID is unique per card type
// (IdSpace::PerCardType) and is what the THETAID fields of a CORD3G of form TABLE name. The cards are kept whole
// (CardRows) next to a column of their TIDs. A TABLE3D names no other card.
class TableScattered3DTable final : public TypedTable<TableScattered3DTable, TableScattered3DCard>
{
public:
    static constexpr std::string_view NAME = "TableScattered3D";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MultiDimensionalTable, base::IdSpace::PerCardType};
    using Format = TableScattered3DFormat;

    [[nodiscard]] static base::EntityId cardId(const TableScattered3DCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableScattered3DCard& card, base::CardKey key);
    [[nodiscard]] TableScattered3DCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TableScattered3DCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TABLE3D refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TableScattered3DCard> m_cards;
};

} // namespace oor::model
