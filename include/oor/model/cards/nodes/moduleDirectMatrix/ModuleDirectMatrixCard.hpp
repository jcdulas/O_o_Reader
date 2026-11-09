#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One term of a column of an MDDMIG matrix: the row, which is a point of a module (the module, the point and its
// component), and the value, real or complex (`imaginary` is the second part, or the phase when the header says POLAR).
struct ModuleDirectMatrixTerm
{
    base::EntityId module = 0;  // MODi: the module that holds the point; 0 is the main Bulk Data
    base::EntityId point = 0;   // Gi: a grid, scalar or extra point of that module
    std::int32_t component = 0; // Ci: 1 to 6 for a grid point, 0 (blank) for a scalar or extra point
    double real = 0.0;          // Ai
    double imaginary = 0.0;     // Bi (0 for a real matrix)

    [[nodiscard]] bool operator==(const ModuleDirectMatrixTerm&) const noexcept = default;
};

// One MDDMIG card: the header of a matrix of direct input at points of several modules, or one column of it. A matrix
// is a header and the columns that carry its name, one card each, in any order after it; this card is kept as it is,
// and the matrix is not assembled (the rules of the DMIG apply to it).
//
// Card layout, header:  MDDMIG  NAME  "0"   IFO  TIN  TOUT  POLAR  .  NCOL          fields 1 to 8
//         column:      MDDMIG  NAME  MODJ  GJ   CJ                                fields 1 to 4
//                      .       .     MOD1  G1   C1   A1   B1                      fields 9 to 14 of each term line
//                      .       .     MOD2  G2   C2   A2   B2   -etc.-
// A term is a line of its own: its first field (9, 17, ...) is blank, MODi, Gi, Ci, Ai and Bi are in the five that
// follow, and the last two are blank. The header is told from a column by field 2: a header has the integer 0 there and
// nothing after field 8, while a column has the module MODJ of its point GJ (0 for the main Bulk Data) and its terms on
// the lines after the first. So a card with MODJ 0 and no terms is a header, and a column of module 0 needs its terms
// (a column with none has nothing to say: each column with a nonzero term has its own entry).
//
// IFO is 1 (square), 2 or 9 (rectangular) or 6 (symmetric); TIN is the type of the input (1 real single, 2 real double,
// 3 complex single, 4 complex double); TOUT the type made (0 set by the system, 1 to 4); POLAR above 0 gives amplitude
// and phase; NCOL is the number of columns of a rectangular matrix (used with IFO 9; kept for every form, as the DMIG
// does). With IFO 2 or 9 the MODJ of a column is the column NUMBER, not a module, and GJ and CJ are ignored,
// so a column keeps a GJ of 0 and the card cannot say that a square matrix needs one (a rule between cards). Bi is
// blank for a real matrix (TIN 1 or 2); a column that has Bi on any term carries the B field on every term.
//
// The modules are plain integers. A point is looked up in this model (a grid, scalar or extra point) when its module is
// 0, the main Bulk Data; the points of other modules are plain IDs, and so is the MODJ that is a column number. The
// card belongs in the main Bulk Data.
struct ModuleDirectMatrixCard
{
    std::string entryName; // upper case, up to 8 characters, the first a letter

    // The column index: MODJ and GJ (0 for the header, or where the format says they are ignored) and CJ.
    base::EntityId columnModuleId = 0;
    base::EntityId nodeJ = 0;
    std::int32_t componentsJ = 0;

    // Header: IFO, TIN, TOUT, POLAR, NCOL (0 when blank).
    std::int32_t matrixShape = 6;
    std::int32_t inputType = 1;
    std::int32_t outputType = 0;
    std::int32_t polarFlag = 0;
    std::int32_t columnCount = 0;

    // Column: the terms, and whether the card carries the B fields.
    std::vector<ModuleDirectMatrixTerm> terms;
    bool imaginary = false;

    [[nodiscard]] bool isHeader() const noexcept { return columnModuleId == 0 && terms.empty(); }
    [[nodiscard]] bool operator==(const ModuleDirectMatrixCard&) const noexcept = default;
};

} // namespace oor::model
