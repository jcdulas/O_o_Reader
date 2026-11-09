#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/matrices/directMatrixNodal/DirectMatrixNodalCard.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One DMIJ card: either the header of a matrix at the js-set of the aerodynamic mesh (the second field is the integer
// 0) or one column of it. Same layout as DMIG, and a term is the same value (`DirectMatrixNodalTerm`); what differs is
// what the matrix is: W2GJ, FA2J and the downwashes and pressures of AEDW and AEPRESS, on the collation degrees of
// freedom of the aerodynamic mesh of CAERO1, CAERO3, CAERO4, CAERO5 and the slender bodies of CAERO2. Its points are
// aerodynamic mesh points, not structural grid points.
//
// Card layout, header:  DMIJ  NAME  "0"  IFO  TIN  TOUT  POLAR  .  NCOL
//         column:      DMIJ  NAME  GJ   CJ   .    G1    C1     A1  B1
//                      .     G2    C2   A2   B2   G3    C3     A3  B3   -etc.-
// Fields 1 to 3 of a column are the name and the column; field 4 is blank and the terms (G, C, A, B) follow four fields
// at a time, wherever they fall on the continuation lines. B is blank for a real matrix (TIN 1 or 2).
struct DirectMatrixAeroJCard
{
    std::string entryName; // upper case, up to 8 characters, the first a letter

    // The column index: GJ and CJ. GJ is 0 for the header card.
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;

    // Header: IFO (1 square, 2 or 9 rectangular, 6 symmetric), TIN (1 real single, 2 real double, 3 complex single, 4
    // complex double), TOUT (0 set by the system, 1 to 4 as TIN), POLAR (0 blank: real and imaginary parts; above 0:
    // amplitude and phase), NCOL (0 blank, the number of columns of a rectangular matrix with IFO 9).
    std::int32_t matrixShape = 9;
    std::int32_t inputType = 1;
    std::int32_t outputType = 0;
    std::int32_t polarFlag = 0;
    std::int32_t columnCount = 0;

    // Column: the terms, and whether the card carries the B fields (a column of a complex matrix: B is written for
    // every term, zero included).
    std::vector<DirectMatrixNodalTerm> terms;
    bool imaginary = false;

    [[nodiscard]] bool isHeader() const noexcept { return nodeJ == 0; }
    [[nodiscard]] bool operator==(const DirectMatrixAeroJCard&) const noexcept = default;
};

} // namespace oor::model
