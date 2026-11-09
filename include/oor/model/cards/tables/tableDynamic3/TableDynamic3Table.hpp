#pragma once

#include "oor/model/cards/tables/TableSupport.hpp"
#include "oor/model/cards/tables/tableDynamic3/TableDynamic3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableDynamic3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableDynamic3Card& card);
    [[nodiscard]] static std::optional<TableDynamic3Card> read(FieldReader& reader);
    static void write(const TableDynamic3Card& card, FieldWriter& writer);
};

// All the TABLED3 cards. The ID is a dynamic table ID, in the ID space the other TABLED cards share
// (IdSpace::DynamicTable). The points of every table are two lists (x, y) in CSR form.
class TableDynamic3Table final : public TypedTable<TableDynamic3Table, TableDynamic3Card>
{
public:
    static constexpr std::string_view NAME = "TableDynamic3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::DynamicTable};
    using Format = TableDynamic3Format;

    [[nodiscard]] static base::EntityId cardId(const TableDynamic3Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableDynamic3Card& card, base::CardKey key);
    [[nodiscard]] TableDynamic3Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableDynamic3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The value of the table at x (NaN where it is not defined). The first form works on a card value, the second on
    // the columns of a row without copying them, the third gives the values at many x.
    [[nodiscard]] static double evaluate(const TableDynamic3Card& card, double x) noexcept;
    [[nodiscard]] double evaluateRow(std::uint32_t row, double x) const;
    void evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& abscissaShiftColumn() noexcept { return m_abscissaShift; }
    [[nodiscard]] Column<double>& abscissaScaleColumn() noexcept { return m_abscissaScale; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<double>& yColumn() noexcept { return m_y; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_abscissaShift;
    Column<double> m_abscissaScale{1.0};
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<double> m_y;
};

} // namespace oor::model
