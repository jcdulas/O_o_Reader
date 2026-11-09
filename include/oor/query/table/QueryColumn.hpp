#pragma once

#include "oor/query/table/ColumnBuffer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// The types a column can have. Each one is an Arrow type with the Arrow memory layout:
//
//   Bool        one bit per value, least significant bit first
//   UInt8, Int32, UInt32, Int64, Float32, Float64      the values, one after the other
//   Utf8        int32 offsets (length + 1 of them) and the characters
//   Dictionary  int32 indices into a Utf8 column of distinct texts (the card type names)
//   FixedList   `width` values of the child column for each row (a node list of four, a position of three)
//   List        int64 offsets and the values of the child column (a node list of any length)
//
// A value that is missing is null: a bit of the validity bitmap, which a column that has no null does not carry. A
// Float32 or Float64 column also holds NaN in the place of a null, so that a view of it as a NumPy array shows the same
// missing values (the result tables of a 32-bit result file are Float32 columns: a result is single precision and widening
// it would double the memory and change nothing); a quantity that is not defined for a card (the volume of a shell) is
// NaN in the arrays of the analysis and is kept as NaN here.
enum class ColumnType : std::uint8_t
{
    Bool,
    UInt8,
    Int32,
    UInt32,
    Int64,
    Float64,
    Utf8,
    Dictionary,
    FixedList,
    List,
    Float32
};

[[nodiscard]] std::string_view columnTypeName(ColumnType type) noexcept;
// The width in bytes of one value of a fixed-width type (0 for the others).
[[nodiscard]] std::size_t valueWidth(ColumnType type) noexcept;
[[nodiscard]] bool isNumeric(ColumnType type) noexcept;  // integers, Float32 and Float64 (not Bool)
[[nodiscard]] bool isFloating(ColumnType type) noexcept; // Float32 and Float64
[[nodiscard]] bool isInteger(ColumnType type) noexcept;
[[nodiscard]] bool isText(ColumnType type) noexcept;

template <typename T>
struct ColumnTypeOf;
template <>
struct ColumnTypeOf<std::uint8_t>
{
    static constexpr ColumnType VALUE = ColumnType::UInt8;
};
template <>
struct ColumnTypeOf<std::int32_t>
{
    static constexpr ColumnType VALUE = ColumnType::Int32;
};
template <>
struct ColumnTypeOf<std::uint32_t>
{
    static constexpr ColumnType VALUE = ColumnType::UInt32;
};
template <>
struct ColumnTypeOf<std::int64_t>
{
    static constexpr ColumnType VALUE = ColumnType::Int64;
};
template <>
struct ColumnTypeOf<float>
{
    static constexpr ColumnType VALUE = ColumnType::Float32;
};
template <>
struct ColumnTypeOf<double>
{
    static constexpr ColumnType VALUE = ColumnType::Float64;
};

// One column of a table: a type, a length, and shared immutable buffers. Copying a column copies no data.
class QueryColumn
{
public:
    QueryColumn() = default;

    [[nodiscard]] ColumnType type() const noexcept { return m_type; }
    [[nodiscard]] std::int64_t length() const noexcept { return m_length; }
    [[nodiscard]] std::size_t size() const noexcept { return static_cast<std::size_t>(m_length); }
    // FixedList: the number of values per row; 0 for the other types.
    [[nodiscard]] std::int32_t width() const noexcept { return m_width; }
    [[nodiscard]] std::int64_t nullCount() const noexcept { return m_nullCount; }
    [[nodiscard]] bool hasNulls() const noexcept { return m_nullCount > 0; }
    [[nodiscard]] bool isNull(std::size_t row) const noexcept
    {
        if (m_nullCount == 0)
            return false;
        const auto* bits = static_cast<const std::uint8_t*>(m_validity.data());
        return ((bits[row >> 3] >> (row & 7)) & 1) == 0;
    }

    [[nodiscard]] const ColumnBuffer& values() const noexcept { return m_values; }
    [[nodiscard]] const ColumnBuffer& offsets() const noexcept { return m_offsets; }
    [[nodiscard]] const ColumnBuffer& validity() const noexcept { return m_validity; }
    // The element column of a list or fixed list; the dictionary (a Utf8 column) of a Dictionary column.
    [[nodiscard]] const std::shared_ptr<const QueryColumn>& child() const noexcept { return m_child; }

    // The values of a fixed-width column as a typed span (Bool: the bytes of the bitmap; Dictionary: the int32
    // indices).
    template <typename T>
    [[nodiscard]] std::span<const T> as() const noexcept
    {
        return m_values.as<T>();
    }

    // The value of a Bool column.
    [[nodiscard]] bool boolAt(std::size_t row) const noexcept
    {
        const auto* bits = static_cast<const std::uint8_t*>(m_values.data());
        return ((bits[row >> 3] >> (row & 7)) & 1) != 0;
    }
    // The text of a Utf8 or Dictionary column (an empty view for a null).
    [[nodiscard]] std::string_view textAt(std::size_t row) const noexcept;
    // The number of values of row `row` of a list or fixed list.
    [[nodiscard]] std::int64_t listLength(std::size_t row) const noexcept;
    // The first value of row `row` of a list or fixed list, as an index into the child column.
    [[nodiscard]] std::int64_t listBegin(std::size_t row) const noexcept;

    // --- making columns ---------------------------------------------------------------------------------------

    template <typename T>
    [[nodiscard]] static QueryColumn primitive(std::vector<T>&& values)
    {
        const auto length = static_cast<std::int64_t>(values.size());
        return fromBuffer(ColumnTypeOf<T>::VALUE, length, ColumnBuffer::adopt(std::move(values)));
    }
    // A column on the buffer of a vector that somebody else shares (a model column), without copy.
    template <typename T>
    [[nodiscard]] static QueryColumn primitiveShared(std::shared_ptr<const std::vector<T>> values)
    {
        const auto length = values ? static_cast<std::int64_t>(values->size()) : 0;
        return fromBuffer(ColumnTypeOf<T>::VALUE, length, ColumnBuffer::share(std::move(values)));
    }
    // A fixed list column on the buffer of a vector of arrays that somebody else shares (a model column), without copy:
    // the arrays have no padding, so the vector is the flat list of the values.
    template <typename T, std::size_t Width>
    [[nodiscard]] static QueryColumn fixedListShared(std::shared_ptr<const std::vector<std::array<T, Width>>> values)
    {
        const auto length = values ? static_cast<std::int64_t>(values->size()) : 0;
        QueryColumn child = fromBuffer(ColumnTypeOf<T>::VALUE, length * static_cast<std::int64_t>(Width),
                                       ColumnBuffer::share(std::move(values)));
        return fixedList(std::move(child), static_cast<std::int32_t>(Width), length);
    }
    [[nodiscard]] static QueryColumn fromBuffer(ColumnType type, std::int64_t length, ColumnBuffer values);

    // A Bool column from one byte per value (0 or 1).
    [[nodiscard]] static QueryColumn boolean(std::span<const std::uint8_t> flags);
    // A Bool column from a bitmap (already packed).
    [[nodiscard]] static QueryColumn booleanFromBits(std::int64_t length, ColumnBuffer bits);
    [[nodiscard]] static QueryColumn utf8(std::span<const std::string> texts);
    [[nodiscard]] static QueryColumn utf8(std::span<const std::string_view> texts);
    // A Utf8 column from offsets (int32, length + 1 of them) and characters.
    [[nodiscard]] static QueryColumn utf8FromBuffers(std::int64_t length, ColumnBuffer offsets,
                                                     ColumnBuffer characters);
    // A Dictionary column: `indices[row]` is the position of the text in `dictionary` (the distinct texts).
    [[nodiscard]] static QueryColumn dictionary(std::vector<std::int32_t>&& indices,
                                                std::span<const std::string> dictionary);
    [[nodiscard]] static QueryColumn dictionaryOf(ColumnBuffer indices, std::int64_t length, QueryColumn dictionary);
    // `width` values of `child` for each row: child.length() == length * width.
    [[nodiscard]] static QueryColumn fixedList(QueryColumn child, std::int32_t width, std::int64_t length);
    // The row `i` has the values child[offsets[i] .. offsets[i + 1]).
    [[nodiscard]] static QueryColumn list(ColumnBuffer offsets, QueryColumn child, std::int64_t length);

    // The same column with a validity bitmap (bit set: the value is there). `nullCount` is the number of bits not set.
    [[nodiscard]] QueryColumn withValidity(ColumnBuffer bits, std::int64_t nullCount) const;
    [[nodiscard]] QueryColumn withoutValidity() const;

private:
    ColumnType m_type = ColumnType::Int32;
    std::int64_t m_length = 0;
    std::int32_t m_width = 0;
    std::int64_t m_nullCount = 0;
    ColumnBuffer m_values;
    ColumnBuffer m_offsets;
    ColumnBuffer m_validity;
    std::shared_ptr<const QueryColumn> m_child;
};

// The bytes needed for `count` bits.
[[nodiscard]] constexpr std::size_t bitmapBytes(std::size_t count) noexcept
{
    return (count + 7) / 8;
}

} // namespace oor::query
