#pragma once

#include "oor/results/core/LabelArray.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oor::results
{

// How trustworthy a table is: `Validated` when its layout was checked against a reference, `NotValidated` when it is
// made from a layout no reference covers, `Experimental` when the layout was inferred (see `statusForWordSize`).
enum class ResultStatus : std::uint8_t
{
    Validated,
    NotValidated,
    Experimental
};

// The status a table has in a file whose words are `wordSize` bytes, given the status its layout earned: `Validated`
// stays, and a layout that is `NotValidated` becomes `Experimental` in a 64-bit file, because nothing checks an 8-byte
// reading that no observed file shows (nothing documents it either: it is the 4-byte layout read with 8-byte words,
// float64 values and interlaced texts). A 32-bit table keeps what its layout earned.
[[nodiscard]] constexpr ResultStatus statusForWordSize(ResultStatus status, int wordSize) noexcept
{
    return wordSize == 8 && status == ResultStatus::NotValidated ? ResultStatus::Experimental : status;
}

[[nodiscard]] std::string_view resultStatusName(ResultStatus status) noexcept;

// The type of the values of a table. A 32-bit file holds single precision numbers, a 64-bit file double precision ones;
// a complex value is a pair (real part, imaginary part) of that precision, which is also the layout of NumPy's
// complex64 and complex128.
enum class ValueType : std::uint8_t
{
    Float32,
    Float64,
    Complex64,
    Complex128
};

[[nodiscard]] std::string_view valueTypeName(ValueType type) noexcept;
// The bytes of one value (a complex value is both parts).
[[nodiscard]] std::size_t valueTypeSize(ValueType type) noexcept;
[[nodiscard]] bool isComplexType(ValueType type) noexcept;

// A text label of the rows of a table, named (the failure theory of a ply, the element name of a grid point force, the
// fibre of a grid point stress). The texts of a table are few and repeat, so a label is a dictionary of the distinct
// texts and one code per row (a table of 1 000 000 rows costs 4 MB, not 40; nothing when the codes are kept from a
// companion package's buffer).
struct ResultText
{
    std::string name;
    std::vector<std::string> dictionary; // the distinct texts (in the order they first appear, when decoded)
    LabelArray<std::uint32_t> codes;     // one per row: the position of its text in the dictionary

    ResultText() = default;
    ResultText(std::string labelName, std::vector<std::string> dictionaryTexts, LabelArray<std::uint32_t> rowCodes)
        : name(std::move(labelName)), dictionary(std::move(dictionaryTexts)), codes(std::move(rowCodes))
    {
    }
    // From one text a row (builds the dictionary).
    ResultText(std::string labelName, const std::vector<std::string>& values);

    [[nodiscard]] std::size_t size() const noexcept { return codes.size(); }
    // The text of row `row`: empty for a code outside the dictionary (the codes kept from a buffer that its owner wrote
    // after the table checked them).
    [[nodiscard]] const std::string& at(std::size_t row) const
    {
        static const std::string EMPTY;
        const std::uint32_t code = codes[row];
        return code < dictionary.size() ? dictionary[code] : EMPTY;
    }
    // One text a row.
    [[nodiscard]] std::vector<std::string> values() const;
    // The text of the rows at `positions`, with the same dictionary.
    [[nodiscard]] ResultText gather(const std::vector<std::uint32_t>& positions) const;
};

// What a table holds once decoded: one immutable block of values, shape (steps, rows, components) in C order, with
// the labels of its three axes. Shared by every view of it (the table, the NumPy arrays that look at it), so it stays
// alive as long as any of them does. The labels of the rows are the table's own, or the buffers of a companion package
// that a table made in memory keeps without a copy, as its values (`LabelArray`).
struct ResultData
{
    ValueType type = ValueType::Float32;
    std::size_t stepCount = 0;
    std::size_t rowCount = 0;
    std::size_t componentCount = 0;
    // The values; 8-byte aligned. `stepCount * rowCount * componentCount` values of `type`. Allocated here, or the
    // buffer of a companion package that a table made in memory keeps without a copy
    // (`MemoryTableInput::sharedValues`): never written once the data is made.
    std::shared_ptr<double[]> storage;

    std::vector<double> steps;           // one per step: the mode number, time, frequency, load factor; NaN when none
    std::vector<double> eigenvalues;     // one per step, or empty
    std::vector<double> eigenvaluesImag; // one per step, or empty
    std::vector<double> cycles;          // one per step, or empty
    LabelArray<std::int32_t> ids; // one per row: the grid point ID (nodal tables), the element ID (element tables)
    LabelArray<std::int32_t> pointTypes; // nodal tables, one per row: 1 grid point, 2 scalar point, 3 extra point, ...
    // Element tables: the rows are places in the elements (a fibre of a corner of a shell, a node of a solid, a station
    // of a beam); each row says where. Empty for nodal tables.
    LabelArray<std::int32_t> nodeIds; // the grid point of the row, 0 at the centroid; empty when the layout has none
    // Grid point forces and the surface grid point stresses: a row is a grid point (`ids`) of an element; this is the
    // element of each row (0 for the totals and the loads that no element carries). Empty when the rows have none.
    LabelArray<std::int32_t> elementIds;
    LabelArray<std::int32_t> layers; // from 1: the fibre of a shell (1 = Z1, 2 = Z2), the ply, the station of a beam
    LabelArray<double> positions;    // the position along the element (beam stations); empty when the layout has none
    std::vector<std::string> components; // one per component
    std::vector<ResultText> texts;       // the text labels of the rows (empty for most tables)

    // The rows of an element table carry layers; those of a nodal table do not.
    [[nodiscard]] bool isElementData() const noexcept { return !layers.empty(); }
    [[nodiscard]] std::size_t valueCount() const noexcept { return stepCount * rowCount * componentCount; }
    [[nodiscard]] std::size_t byteCount() const noexcept { return valueCount() * valueTypeSize(type); }
    [[nodiscard]] const std::byte* bytes() const noexcept { return reinterpret_cast<const std::byte*>(storage.get()); }
    [[nodiscard]] std::byte* mutableBytes() noexcept { return reinterpret_cast<std::byte*>(storage.get()); }
    // The values as an array of `T` (float, double; for complex types, the interleaved real and imaginary parts).
    template <typename T>
    [[nodiscard]] std::span<const T> values() const noexcept
    {
        return std::span<const T>(reinterpret_cast<const T*>(storage.get()), byteCount() / sizeof(T));
    }
    template <typename T>
    [[nodiscard]] std::span<T> mutableValues() noexcept
    {
        return std::span<T>(reinterpret_cast<T*>(storage.get()), byteCount() / sizeof(T));
    }

    // Allocates `storage` for the shape given (not initialised).
    void allocate();
};

} // namespace oor::results
