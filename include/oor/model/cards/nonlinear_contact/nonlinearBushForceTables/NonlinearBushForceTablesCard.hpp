#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// How the 6 x 6 matrix of force tables of an LBSH3DG is given (the FORM field). The enumerators are numbered in the
// order of NONLINEAR_BUSH_FORM_WORDS.
enum class BushingForm : std::uint8_t
{
    Symmetric, // SYM (the default): the terms on and under the diagonal
    Diagonal,  // DIAG: the six terms of the diagonal
    Asymmetric // ASYM: all 36 terms
};

inline constexpr std::array<std::string_view, 3> NONLINEAR_BUSH_FORM_WORDS{"SymmetricForm", "DiagonalForm",
                                                                           "AsymmetricForm"};

// The rows and columns of the matrix of an LBSH3DG: the six degrees of freedom of a grid point.
inline constexpr std::size_t BUSHING_MATRIX_SIZE = 6;

// Whether the form has a term in row `row` and column `column` (both from 0): every term for ASYM, the terms of the
// lower triangle (the diagonal included) for SYM, the diagonal for DIAG.
[[nodiscard]] constexpr bool bushingTermIsUsed(BushingForm form, std::size_t row, std::size_t column) noexcept
{
    switch (form)
    {
    case BushingForm::Asymmetric:
        return true;
    case BushingForm::Symmetric:
        return column <= row;
    case BushingForm::Diagonal:
        break;
    }
    return column == row;
}

// The field of the card (field 1 is the LID) that holds the term of row `row` and column `column` (from 0), whose term
// the form has. The first line of the card holds LID, FORM, MIN_FREQ and MAX_FREQ and the matrix starts on the second
// line (field 9). ASYM and SYM put the term Pij of row i and column j in column j of the line of the row i, so the
// triangle of SYM is the lower left half of the grid of ASYM; DIAG puts the six diagonal terms P11 to P66 on the first
// matrix line.
[[nodiscard]] constexpr int bushingTermField(BushingForm form, std::size_t row, std::size_t column) noexcept
{
    constexpr int FIRST_MATRIX_FIELD = 9;
    constexpr int FIELDS_PER_LINE = 8;
    if (form == BushingForm::Diagonal)
        return FIRST_MATRIX_FIELD + static_cast<int>(row);
    return FIRST_MATRIX_FIELD + FIELDS_PER_LINE * static_cast<int>(row) + static_cast<int>(column);
}

// One LBSH3DG card: the force tables of the NLBSH3D nonlinear loads between two grid points, as a matrix of tables over
// the rotor speed (SOL 400 rotor dynamics).
//
// Card layout (the form is the word FORM in field 2; the matrix starts on the second line, in field 9):
//   LBSH3DG  LID  FORM  MIN_FREQ  MAX_FREQ                        fields 1 to 4
//   ASYM: six lines, the row i on the line i: Pi1 Pi2 Pi3 Pi4 Pi5 Pi6     fields 9 to 14, 17 to 22, ... 49 to 54
//   SYM:  the lower triangle, the row i on the line i: Pi1 ... Pii         fields 9, 17 and 18, 25 to 27, ... 49 to 54
//   DIAG: one line: P11 P22 P33 P44 P55 P66                                fields 9 to 14
// LID is the number of the force group that an NLBSH3D names (the LID-1 and LID-2 of the load), required. FORM is SYM
// when blank. MIN_FREQ and MAX_FREQ are the lowest and highest rotor speed for which the tables are valid;
// MAX_FREQ is required and above 0.0, MIN_FREQ has no default on the format and is 0.0 when blank. Every Pij is
// the ID of a TABLED5 (the format: the table that gives the TABLED1 of the force against the displacement or velocity,
// for each rotor speed): a table in the space of the dynamic tables. A term that the form has and that is left blank is
// kept as blank (0: no table, the term is a zero force), though the format calls the terms of the form "needed"; a term
// that the form has no cell for (an upper triangle term of a SYM card, an off-diagonal term of a DIAG card) is
// refused. The layout is the one of the three forms of the format.
//
// The card is the group that an NLBSH3D names by LID, and has an ID space of its own (one card per LID).
struct NonlinearBushForceTablesCard
{
    using Terms = std::array<std::array<base::EntityId, BUSHING_MATRIX_SIZE>, BUSHING_MATRIX_SIZE>;

    base::EntityId id = 0;
    BushingForm form = BushingForm::Symmetric;
    double minimumFreq = 0.0;
    double maximumFreq = 0.0;
    Terms tables = {}; // tables[i][j] is the TABLED5 of the term P(i+1)(j+1); 0 where blank or not in the form
};

} // namespace oor::model
