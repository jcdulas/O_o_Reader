#pragma once

#include "oor/model/cards/tables/TableSupport.hpp"
#include "oor/model/cards/tables/tableStress1/TableStress1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableStress1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableStress1Card& card);
    [[nodiscard]] static std::optional<TableStress1Card> read(FieldReader& reader);
    static void write(const TableStress1Card& card, FieldWriter& writer);
};

// All the TABLES1 cards. The ID is in the stress table space it shares with TABLEST (IdSpace::StressTable). The points
// of every table are two lists (x, y) in CSR form.
class TableStress1Table final : public TypedTable<TableStress1Table, TableStress1Card>
{
public:
    static constexpr std::string_view NAME = "TableStress1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::StressTable};
    using Format = TableStress1Format;

    TableStress1Table() : m_kind(1) {}

    [[nodiscard]] static base::EntityId cardId(const TableStress1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableStress1Card& card, base::CardKey key);
    [[nodiscard]] TableStress1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableStress1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The value of the table at x (NaN where it is not defined). The first form works on a card value, the second on
    // the columns of a row without copying them, the third gives the values at many x.
    [[nodiscard]] static double evaluate(const TableStress1Card& card, double x) noexcept;
    [[nodiscard]] double evaluateRow(std::uint32_t row, double x) const;
    void evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<double>& yColumn() noexcept { return m_y; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_kind;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<double> m_y;
};

} // namespace oor::model
