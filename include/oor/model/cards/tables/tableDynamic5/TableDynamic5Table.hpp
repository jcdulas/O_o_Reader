#pragma once

#include "oor/model/cards/tables/tableDynamic5/TableDynamic5Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

class Model;

struct TableDynamic5Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableDynamic5Card& card);
    [[nodiscard]] static std::optional<TableDynamic5Card> read(FieldReader& reader);
    static void write(const TableDynamic5Card& card, FieldWriter& writer);
};

// All the TABLED5 cards. The ID is a dynamic table ID, in the ID space the TABLED1 to TABLED4 cards share
// (IdSpace::DynamicTable); the tables of the pairs are references to them (ReferenceTarget::DynamicTable). The pairs of
// every table are two lists (x, table ID) in CSR form.
class TableDynamic5Table final : public TypedTable<TableDynamic5Table, TableDynamic5Card>
{
public:
    static constexpr std::string_view NAME = "TableDynamic5";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::DynamicTable};
    using Format = TableDynamic5Format;

    [[nodiscard]] static base::EntityId cardId(const TableDynamic5Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableDynamic5Card& card, base::CardKey key);
    [[nodiscard]] TableDynamic5Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableDynamic5Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The value of f(x, y): the tables of the pairs are evaluated at y and interpolated linearly in x (the first and
    // the last segment extended outside the x of the card, as TABLED1 does). NaN where it is not defined: x or y is
    // NaN, or a table of the pairs is missing in `model` or is not a TABLED1 to TABLED4.
    [[nodiscard]] static double evaluate(const Model& model, const TableDynamic5Card& card, double x, double y);
    [[nodiscard]] double evaluateRow(const Model& model, std::uint32_t row, double x, double y) const;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& xColumn() noexcept { return m_x; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& tablesColumn() noexcept { return m_tables; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_x;
    VariableLengthColumn<base::EntityId> m_tables;
};

} // namespace oor::model
