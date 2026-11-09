#pragma once

#include "oor/query/apply/CellValue.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace oor::query
{

// The changes made to one column of a table: which cells were set, and the value each was given. The column itself is
// an immutable Arrow column and stays as it is: a cell that was never set reads from it, a cell that was set reads from
// here, and `materialize` makes the column with both put together (a new buffer; the old one stays valid for whoever
// holds it).
//
// The values are kept by the kind of the column:
//   Real, Integer         one value per row of the column (allocated at the first change); an integer is kept as int64
//                         whatever the width of the column, and checked against the range of the column's type
//   Text                  the texts of the changed cells
//   RealList, IntegerList a fixed list (nodes, xyz) keeps its values in one block of `rows * width`; a list of any
//   length
//                         keeps the changed cells' lists
// A cell can be set to a null. Setting a cell to the value it already has is a change all the same (a table that was
// told to write a value writes it; `EditableTable::update` is the one that compares).
class ColumnEdits
{
public:
    enum class Kind : std::uint8_t
    {
        Real,
        Integer,
        Text,
        RealList,
        IntegerList
    };

    // True for the columns whose values can be set: numbers, bools, texts, and lists of numbers.
    [[nodiscard]] static bool supports(const QueryColumn& column) noexcept;

    // Throws QueryTypeError for a column that `supports` says no to.
    explicit ColumnEdits(const QueryColumn& base);

    [[nodiscard]] Kind kind() const noexcept { return m_kind; }
    // The type of the column (Float64, Int32, ...), or the type of the values of a list.
    [[nodiscard]] ColumnType valueType() const noexcept { return m_valueType; }
    // The number of values of a fixed list, 0 for the other kinds.
    [[nodiscard]] std::int32_t width() const noexcept { return m_width; }
    [[nodiscard]] std::size_t rowCount() const noexcept { return m_state.size(); }

    [[nodiscard]] std::size_t changedCount() const noexcept { return m_changed; }
    [[nodiscard]] bool isChanged(std::size_t row) const noexcept { return m_state[row] != 0; }
    // The rows that were set, in increasing order.
    [[nodiscard]] std::vector<std::uint32_t> changedRows() const;

    // Throws QueryTypeError, saying what the column expects, when `value` does not fit the column.
    void check(const CellValue& value) const;
    // Sets one cell (after `check`). Throws QueryError for a row outside the column.
    void set(std::size_t row, const CellValue& value);
    // Sets the cells `rows` of a Real or Integer column. `values` has one value for every row, or one value for all of
    // them. Nothing is changed when a value does not fit or a row is outside the column.
    void setReals(std::span<const std::uint32_t> rows, std::span<const double> values);
    void setIntegers(std::span<const std::uint32_t> rows, std::span<const std::int64_t> values);
    // Takes a cell back to the value of the column.
    void unset(std::size_t row) noexcept;
    void clear();

    // The value of a cell now: the one it was set to, or the one of `base`.
    [[nodiscard]] CellValue value(std::size_t row, const QueryColumn& base) const;
    // The value the cell would have if it had been set: `base` must be the column the edits were made for.
    [[nodiscard]] QueryColumn materialize(const QueryColumn& base) const;

private:
    static constexpr std::uint8_t UNCHANGED = 0;
    static constexpr std::uint8_t SET = 1;
    static constexpr std::uint8_t SET_NULL = 2;

    void touch(std::size_t row, std::uint8_t state) noexcept;
    void ensureNumbers();
    void checkRows(std::span<const std::uint32_t> rows) const;
    [[nodiscard]] QueryColumn withNulls(QueryColumn column, const QueryColumn& base) const;
    [[nodiscard]] QueryColumn materializeScalar(const QueryColumn& base) const;
    [[nodiscard]] QueryColumn materializeText(const QueryColumn& base) const;
    [[nodiscard]] QueryColumn materializeList(const QueryColumn& base) const;

    Kind m_kind = Kind::Real;
    ColumnType m_columnType = ColumnType::Float64; // the type of the column
    ColumnType m_valueType = ColumnType::Float64;  // of the values (the child of a list)
    std::int32_t m_width = 0;
    std::vector<std::uint8_t> m_state; // UNCHANGED, SET or SET_NULL for each row
    std::size_t m_changed = 0;
    std::size_t m_nulls = 0;
    std::vector<double> m_reals;
    std::vector<std::int64_t> m_integers;
    std::unordered_map<std::uint32_t, std::string> m_texts;
    std::unordered_map<std::uint32_t, std::vector<double>> m_realLists;
    std::unordered_map<std::uint32_t, std::vector<std::int64_t>> m_integerLists;
};

} // namespace oor::query
