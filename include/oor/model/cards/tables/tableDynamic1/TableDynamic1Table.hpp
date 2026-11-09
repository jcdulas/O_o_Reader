#pragma once

#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableDynamic1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableDynamic1Card& card);
    [[nodiscard]] static std::optional<TableDynamic1Card> read(FieldReader& reader);
    static void write(const TableDynamic1Card& card, FieldWriter& writer);
};

// All the TABLED1 cards. The ID is a dynamic table ID, in the ID space the TABLED2 to TABLED4 cards share
// (IdSpace::DynamicTable). The points of every table are two lists (x, y) in CSR form.
class TableDynamic1Table final : public TypedTable<TableDynamic1Table, TableDynamic1Card>
{
public:
    static constexpr std::string_view NAME = "TableDynamic1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::DynamicTable};
    using Format = TableDynamic1Format;

    [[nodiscard]] static base::EntityId cardId(const TableDynamic1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableDynamic1Card& card, base::CardKey key);
    [[nodiscard]] TableDynamic1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableDynamic1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The value of the table at x (NaN where it is not defined). The first form works on a card value, the second on
    // the columns of a row without copying them, the third gives the values at many x.
    [[nodiscard]] static double evaluate(const TableDynamic1Card& card, double x) noexcept;
    [[nodiscard]] double evaluateRow(std::uint32_t row, double x) const;
    void evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& xAxisKindColumn() noexcept { return m_xAxisKind; }
    [[nodiscard]] Column<std::uint8_t>& yAxisKindColumn() noexcept { return m_yAxisKind; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<double>& yColumn() noexcept { return m_y; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_xAxisKind;
    Column<std::uint8_t> m_yAxisKind;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<double> m_y;
};

} // namespace oor::model
