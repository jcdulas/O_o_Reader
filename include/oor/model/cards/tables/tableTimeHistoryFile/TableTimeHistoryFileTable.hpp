#pragma once

#include "oor/model/cards/tables/tableTimeHistoryFile/TableTimeHistoryFileCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableTimeHistoryFileFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableTimeHistoryFileCard& card);
    [[nodiscard]] static std::optional<TableTimeHistoryFileCard> read(FieldReader& reader);
    static void write(const TableTimeHistoryFileCard& card, FieldWriter& writer);
};

// All the time-history-file table cards. The ID is a table number in the space the TABLED1 to TABLED5 cards share
// (IdSpace::DynamicTable, category Table): such a card may be named by anything that names a TABLED1, and
// may not have a TABLED1's number. The UID is a UserDefinedName reference (its absolute value: a negative UID inverts
// the pairs). The cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class TableTimeHistoryFileTable final : public TypedTable<TableTimeHistoryFileTable, TableTimeHistoryFileCard>
{
public:
    static constexpr std::string_view NAME = "TableTimeHistoryFile";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::DynamicTable};
    using Format = TableTimeHistoryFileFormat;

    [[nodiscard]] static base::EntityId cardId(const TableTimeHistoryFileCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableTimeHistoryFileCard& card, base::CardKey key);
    [[nodiscard]] TableTimeHistoryFileCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TableTimeHistoryFileCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TableTimeHistoryFileCard> m_cards;
};

} // namespace oor::model
