#pragma once

#include "oor/model/cards/tables/tableModalDamping1/TableModalDamping1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableModalDamping1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableModalDamping1Card& card);
    [[nodiscard]] static std::optional<TableModalDamping1Card> read(FieldReader& reader);
    static void write(const TableModalDamping1Card& card, FieldWriter& writer);
};

// All the TABDMP1 cards. The ID is a damping table ID (IdSpace::HybridDampingRotorOptionsTable): the TABLEDi tables
// have a space of their own (a deck may have a TABLED1 and a TABDMP1 of the same ID), and the SDAMPING command selects
// either. The points of every table are two lists (x, y) in CSR form.
class TableModalDamping1Table final : public TypedTable<TableModalDamping1Table, TableModalDamping1Card>
{
public:
    static constexpr std::string_view NAME = "TableModalDamping1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::HybridDampingRotorOptionsTable};
    using Format = TableModalDamping1Format;

    [[nodiscard]] static base::EntityId cardId(const TableModalDamping1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableModalDamping1Card& card, base::CardKey key);
    [[nodiscard]] TableModalDamping1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableModalDamping1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The damping at natural frequency x (NaN where it is not defined): linear between the points, the first and the
    // last segment extended outside (as TABLED1), the mean of the two values at a jump. The three forms are the same as
    // for TABLED1.
    [[nodiscard]] static double evaluate(const TableModalDamping1Card& card, double x) noexcept;
    [[nodiscard]] double evaluateRow(std::uint32_t row, double x) const;
    void evaluateRow(std::uint32_t row, std::span<const double> x, std::span<double> y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<double>& yColumn() noexcept { return m_y; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_type;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<double> m_y;
};

} // namespace oor::model
