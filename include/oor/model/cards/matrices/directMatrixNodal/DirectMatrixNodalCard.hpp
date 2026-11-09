#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One term of a column of a DMIG matrix: the row (a grid, scalar or extra point and a component) and the value, real
// or complex (`imaginary` is the second part, or the phase when the header says POLAR).
struct DirectMatrixNodalTerm
{
    base::EntityId point = 0;   // Gi
    std::int32_t component = 0; // Ci: 1 to 6 for a grid point, 0 (blank) for a scalar or extra point
    double real = 0.0;          // Ai
    double imaginary = 0.0;     // Bi (0 for a real matrix)

    [[nodiscard]] bool operator==(const DirectMatrixNodalTerm&) const noexcept = default;
};

// One DMIG card: either the header of a matrix (the second field is the integer 0) or one column of it. A matrix is a
// header and the columns that carry its name, one card each, in any order after it; the matrix as one object (the
// terms of every column, in COO form) is assembled by `directMatrixNodalTerms` and `directMatrixNodalCoo`
// (MatrixCards.hpp).
//
// Card layout, header:  DMIG  NAME  "0"  IFO  TIN  TOUT  POLAR  .  NCOL
//         column:      DMIG  NAME  GJ   CJ   .    G1    C1     A1  B1
//                      .     G2    C2   A2   B2   G3    C3     A3  B3   -etc.-
// Fields 1 to 3 of a column are the name and the column; field 4 is blank and the terms (G, C, A, B) follow four fields
// at a time, wherever they fall on the continuation lines. B is blank for a real matrix (TIN 1 or 2). NCOL is read for
// every form (the format reference uses it for IFO 9): it is kept for every form.
struct DirectMatrixNodalCard
{
    std::string entryName; // upper case, up to 8 characters, the first a letter

    // The column index: GJ and CJ. GJ is 0 for the header card.
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;

    // Header: IFO (1 square, 2 or 9 rectangular, 6 symmetric), TIN (1 real single, 2 real double, 3 complex single, 4
    // complex double), TOUT (0 set by the system, 1 to 4 as TIN), POLAR (0 blank: real and imaginary parts; above 0:
    // amplitude and phase), NCOL (0 blank).
    std::int32_t matrixShape = 6;
    std::int32_t inputType = 1;
    std::int32_t outputType = 0;
    std::int32_t polarFlag = 0;
    std::int32_t columnCount = 0;

    // Column: the terms, and whether the card carries the B fields (a column of a complex matrix: B is written for
    // every term, zero included).
    std::vector<DirectMatrixNodalTerm> terms;
    bool imaginary = false;

    [[nodiscard]] bool isHeader() const noexcept { return nodeJ == 0; }
    [[nodiscard]] bool operator==(const DirectMatrixNodalCard&) const noexcept = default;
};

} // namespace oor::model
