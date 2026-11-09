#pragma once

#include "oor/model/cards/tables/tableRandomSpectrum1/TableRandomSpectrum1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct TableRandomSpectrum1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableRandomSpectrum1Card& card);
    [[nodiscard]] static std::optional<TableRandomSpectrum1Card> read(FieldReader& reader);
    static void write(const TableRandomSpectrum1Card& card, FieldWriter& writer);
};

// All the TABRND1 cards. The ID is a random table ID, in the ID space that TABRNDG shares (IdSpace::RandomTable): the
// TID of a RANDPS card names either. The points of every table are two lists (x, y) in CSR form.
class TableRandomSpectrum1Table final : public TypedTable<TableRandomSpectrum1Table, TableRandomSpectrum1Card>
{
public:
    static constexpr std::string_view NAME = "TableRandomSpectrum1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::RandomTable};
    using Format = TableRandomSpectrum1Format;

    [[nodiscard]] static base::EntityId cardId(const TableRandomSpectrum1Card& card) noexcept { return card.id; }
    std::uint32_t append(const TableRandomSpectrum1Card& card, base::CardKey key);
    [[nodiscard]] TableRandomSpectrum1Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableRandomSpectrum1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    // The power spectral density at frequency x (NaN where it is not defined), as TABLED1 gives its value: interpolated
    // in the scales of XAXIS and YAXIS, the first and the last segment extended outside, the mean at a jump.
    [[nodiscard]] static double evaluate(const TableRandomSpectrum1Card& card, double x) noexcept;
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
