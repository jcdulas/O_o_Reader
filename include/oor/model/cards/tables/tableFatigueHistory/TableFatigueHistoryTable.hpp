#pragma once

#include "oor/model/cards/tables/tableFatigueHistory/TableFatigueHistoryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableFatigueHistoryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableFatigueHistoryCard& card);
    [[nodiscard]] static std::optional<TableFatigueHistoryCard> read(FieldReader& reader);
    static void write(const TableFatigueHistoryCard& card, FieldWriter& writer);
};

// All the TABLFTG cards, in the category FatigueTable: the TID is unique per card type (IdSpace::PerCardType) and is
// what an FTGLOAD names. The values of every table are one list in CSR form next to the column of the TIDs, so the
// query engine sees both. A TABLFTG names no other card.
class TableFatigueHistoryTable final : public TypedTable<TableFatigueHistoryTable, TableFatigueHistoryCard>
{
public:
    static constexpr std::string_view NAME = "TableFatigueHistory";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FatigueTable, base::IdSpace::PerCardType};
    using Format = TableFatigueHistoryFormat;

    [[nodiscard]] static base::EntityId cardId(const TableFatigueHistoryCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableFatigueHistoryCard& card, base::CardKey key);
    [[nodiscard]] TableFatigueHistoryCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableFatigueHistoryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TABLFTG refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& ordinateColumn() noexcept { return m_ordinate; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_ordinate;
};

} // namespace oor::model
