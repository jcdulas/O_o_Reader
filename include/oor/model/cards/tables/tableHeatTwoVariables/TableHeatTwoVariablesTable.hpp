#pragma once

#include "oor/model/cards/tables/tableHeatTwoVariables/TableHeatTwoVariablesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableHeatTwoVariablesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableHeatTwoVariablesCard& card);
    [[nodiscard]] static std::optional<TableHeatTwoVariablesCard> read(FieldReader& reader);
    static void write(const TableHeatTwoVariablesCard& card, FieldWriter& writer);
};

// All the TABLEHT cards. The ID is in the heat table space it shares with TABLEH1 (IdSpace::HeatTable), and the table
// IDs of the pairs are references to TABLEH1 cards of that space. The values of x and the table IDs of every card are
// two lists in CSR form.
class TableHeatTwoVariablesTable final : public TypedTable<TableHeatTwoVariablesTable, TableHeatTwoVariablesCard>
{
public:
    static constexpr std::string_view NAME = "TableHeatTwoVariables";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::HeatTable};
    using Format = TableHeatTwoVariablesFormat;

    [[nodiscard]] static base::EntityId cardId(const TableHeatTwoVariablesCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableHeatTwoVariablesCard& card, base::CardKey key);
    [[nodiscard]] TableHeatTwoVariablesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableHeatTwoVariablesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& tablesColumn() noexcept { return m_tables; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<base::EntityId> m_tables;
};

} // namespace oor::model
