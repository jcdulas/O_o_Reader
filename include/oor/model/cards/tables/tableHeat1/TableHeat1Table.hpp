#pragma once

#include "oor/model/cards/tables/TableSupport.hpp"
#include "oor/model/cards/tables/tableHeat1/TableHeat1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableHeat1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableHeat1Card& card);
    [[nodiscard]] static std::optional<TableHeat1Card> read(FieldReader& reader);
    static void write(const TableHeat1Card& card, FieldWriter& writer);
};

// All the TABLEH1 cards. The ID is in the heat table space it shares with TABLEHT (IdSpace::HeatTable). The points of
// every table are two lists (x, y) in CSR form.
class TableHeat1Table final : public TypedTable<TableHeat1Table, TableHeat1Card>
{
public:
    static constexpr std::string_view NAME = "TableHeat1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::HeatTable};
    using Format = TableHeat1Format;

    [[nodiscard]] static base::EntityId cardId(const TableHeat1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableHeat1Card& card, base::CardKey key);
    [[nodiscard]] TableHeat1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableHeat1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The value of the table at x (NaN where it is not defined): linear between the points, the value of the nearest
    // end point outside them. The first form works on a card value, the second on the columns of a row without copying
    // them, the third gives the values at many x.
    [[nodiscard]] static double evaluate(const TableHeat1Card& card, double x) noexcept;
    [[nodiscard]] double evaluateRow(std::uint32_t row, double x) const;
    void evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<double>& yColumn() noexcept { return m_y; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<double> m_y;
};

} // namespace oor::model
