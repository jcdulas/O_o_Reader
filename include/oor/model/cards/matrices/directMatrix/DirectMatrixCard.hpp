#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// What a field of a DMI column entry is after the first row number I1.
enum class DirectMatrixTokenKind : std::uint8_t
{
    Value, // an element of the column: A (and B after it for a complex matrix)
    Row,   // an integer: the row number where a new group of elements starts (I2, ...)
    Thru   // the word THRU and the row number after it: the element before is repeated down to that row
};

// One field (or, for THRU, two) of a column entry.
struct DirectMatrixToken
{
    DirectMatrixTokenKind kind = DirectMatrixTokenKind::Value;
    double value = 0.0;   // Value
    std::int32_t row = 0; // Row: I; Thru: the last row

    [[nodiscard]] bool operator==(const DirectMatrixToken&) const noexcept = default;
};

// One DMI card: either the header of a matrix (the second field is the integer 0) or one column of it. The elements of
// a column are kept as written (groups of elements after a row number, THRU runs): whether a value is real or the real
// part of a complex pair depends on TIN of the header, which is another card, so a column is turned into elements by
// the matrix (`directMatrixCoo`, MatrixCards.hpp), not by the card.
//
// Card layout, header:  DMI  NAME  "0"  FORM  TIN  TOUT  .  M  N
//         column:      DMI  NAME  J    I1    A(I1,J)  A(I1+1,J)  -etc.-   I2  A(I2,J)  -etc.-
//         complex:     DMI  NAME  J    I1    A(I1,J)  B(I1,J)  A(I1+1,J)  B(I1+1,J)  -etc.-
// FORM: 1 square, 2 rectangular, 3 diagonal, 4 lower triangular factor, 5 upper triangular factor, 6 symmetric,
// 8 identity. TIN: 1 real single, 2 real double, 3 complex single, 4 complex double. TOUT: 0 set by the system, 1 to 4
// as TIN. M rows and N columns (N is not used for FORM 3 and 8).
struct DirectMatrixCard
{
    std::string entryName; // upper case, up to 8 characters, the first a letter

    // The column number J; 0 for the header card.
    std::int32_t columnIndex = 0;

    // Header.
    std::int32_t matrixForm = 2;
    std::int32_t inputType = 1;
    std::int32_t outputType = 0;
    std::int32_t rows = 0;    // M
    std::int32_t columns = 0; // N (0 blank)

    // Column: the first row I1 and the fields after it.
    std::int32_t firstRow = 0;
    std::vector<DirectMatrixToken> tokens;

    [[nodiscard]] bool isHeader() const noexcept { return columnIndex == 0; }
    [[nodiscard]] bool operator==(const DirectMatrixCard&) const noexcept = default;
};

} // namespace oor::model
