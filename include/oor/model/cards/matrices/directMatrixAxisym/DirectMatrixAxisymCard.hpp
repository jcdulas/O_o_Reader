#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace oor::model
{

// One term of a column of a DMIAX matrix: the row (a grid, scalar, fluid, pressure, free surface or extra point, its
// component and its harmonic number) and the value, real or complex.
struct DirectMatrixAxisymTerm
{
    base::EntityId point = 0;   // Gi
    std::int32_t component = 0; // Ci: 1 to 6 for a grid point, 0 (blank) for a scalar, fluid or extra point
    std::int32_t harmonic = 0;  // Ni: the harmonic number of a RINGFL point (negative: sine series), else blank
    double real = 0.0;          // Ai
    double imaginary = 0.0;     // Bi (0 for a real matrix)

    [[nodiscard]] bool operator==(const DirectMatrixAxisymTerm&) const noexcept = default;
};

// One DMIAX card: the matrix input of an axisymmetric analysis (a model with an AXIF entry), either the header of a
// matrix (the second field is the integer 0) or one column of it. A matrix is a header and the columns that carry its
// name, one card each, in any order; the matrix as one object is assembled by `directMatrixAxisymTerms` and
// `directMatrixAxisymCoo` (MatrixCards.hpp).
//
// Card layout, header:  DMIAX  NAME  "0"  IFO  TIN  TOUT
//         column:      DMIAX  NAME  GJ   CJ   NJ
//                      .      G1    C1   N1   A1   B1
//                      .      G2    C2   N2   A2   B2   -etc.-
// A column entry is its first line and then one continuation line for each term: G, C, N, A and B in the first five
// fields of the line (fields 9 to 13 of the card, 17 to 21 for the second term, and so on). B is blank for a real
// matrix (TIN 1), and N for every point that is not a RINGFL point. The matrix terms are the harmonics: a blank N is
// kept apart from the harmonic 0 (`NO_HARMONIC`).
struct DirectMatrixAxisymCard
{
    static constexpr std::int32_t NO_HARMONIC = std::numeric_limits<std::int32_t>::min(); // the N field is blank

    std::string entryName; // upper case, up to 8 characters, the first a letter

    // The column index: GJ, CJ and NJ. GJ is 0 for the header card.
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;
    std::int32_t harmonicJ = NO_HARMONIC;

    // Header: IFO (1 square, 2 rectangular, 6 symmetric), TIN (1 real, 3 complex; single precision), TOUT (0 set by the
    // system, 1 real single, 2 real double, 3 complex single, 4 complex double).
    std::int32_t matrixShape = 1;
    std::int32_t inputType = 1;
    std::int32_t outputType = 0;

    // Column: the terms, and whether the card carries the B fields (a column of a complex matrix: B is written for
    // every term, zero included).
    std::vector<DirectMatrixAxisymTerm> terms;
    bool imaginary = false;

    [[nodiscard]] bool isHeader() const noexcept { return nodeJ == 0; }
    [[nodiscard]] bool operator==(const DirectMatrixAxisymCard&) const noexcept = default;
};

} // namespace oor::model
