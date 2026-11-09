#pragma once

#include "oor/model/cards/tables/tableMultiVariable1/TableMultiVariable1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableMultiVariable1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableMultiVariable1Card& card);
    [[nodiscard]] static std::optional<TableMultiVariable1Card> read(FieldReader& reader);
    static void write(const TableMultiVariable1Card& card, FieldWriter& writer);
};

// All the TABL3D1 cards, in the category MultiDimensionalTable: the ITID is unique per card type
// (IdSpace::PerCardType) and is what the SOL 400 materials, the contact cards and a CORD3G name. The cards are kept
// whole (CardRows) next to a column of their ITIDs. A TABL3D1 names no other card: the ITIDS and ITIDB of its variables
// are not used by the program, so they are not visited.
class TableMultiVariable1Table final : public TypedTable<TableMultiVariable1Table, TableMultiVariable1Card>
{
public:
    static constexpr std::string_view NAME = "TableMultiVariable1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MultiDimensionalTable, base::IdSpace::PerCardType};
    using Format = TableMultiVariable1Format;

    [[nodiscard]] static base::EntityId cardId(const TableMultiVariable1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableMultiVariable1Card& card, base::CardKey key);
    [[nodiscard]] TableMultiVariable1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TableMultiVariable1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TABL3D1 refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TableMultiVariable1Card> m_cards;
};

} // namespace oor::model
