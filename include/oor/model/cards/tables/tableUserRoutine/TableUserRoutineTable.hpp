#pragma once

#include "oor/model/cards/tables/tableUserRoutine/TableUserRoutineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableUserRoutineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableUserRoutineCard& card);
    [[nodiscard]] static std::optional<TableUserRoutineCard> read(FieldReader& reader);
    static void write(const TableUserRoutineCard& card, FieldWriter& writer);
};

// All the TABLUDS cards. The ID is a table number in the space the TABLED1 to TABLED5 cards share
// (IdSpace::DynamicTable, category Table): the format reference says tables and user defined functions are one group,
// so a TABLUDS and a TABLED1 may not have the same number, and a field that names a dynamic table finds a TABLUDS too.
// The cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class TableUserRoutineTable final : public TypedTable<TableUserRoutineTable, TableUserRoutineCard>
{
public:
    static constexpr std::string_view NAME = "TableUserRoutine";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::DynamicTable};
    using Format = TableUserRoutineFormat;

    [[nodiscard]] static base::EntityId cardId(const TableUserRoutineCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableUserRoutineCard& card, base::CardKey key);
    [[nodiscard]] TableUserRoutineCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TableUserRoutineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TABLUDS refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TableUserRoutineCard> m_cards;
};

} // namespace oor::model
