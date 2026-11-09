#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

#include <vector>

namespace oor::model
{

// One TABL3D0 card (SOL 400): the simplest of the three forms of the TABL3D entry, a table of one independent
// variable. The entry gives a quantity such as a modulus, a yield stress or a friction coefficient as a function of
// strain, temperature, strain rate and the like; the SOL 400 materials and the contact cards name it by its ITID.
//
// Card layout (the first form of the entry, whose name field reads TABL3D0):
//   TABL3D0  ITID  (blank)  KIND  EXTRP  ITIDS  ITIDB  SM           fields 1 to 7 (field 2 is not used)
//            X1    Y1       X2    Y2     X3     Y3     X4   Y4      from field 9, four pairs to a line
//            X5    Y5       -etc.-
// There is no ENDT: the pairs end with the card.
//
// KIND says what X is: a positive integer from the table of independent variable types of the format reference
// (numbered 1 to 76: time, the increment, the coordinates, the angle, frequency, temperature, plastic and creep
// strains, stresses and their rates, state variables, damage, ...; some are marked as not available in SOL 400). The
// card checks the sign of KIND and nothing else, and does not evaluate the table. EXTRP is 1 to hold the end values
// outside the data and 2 (the default) to continue the first and the last segment. SM is 1 to smooth the data and 0
// (the default) not to. ITIDS and ITIDB are meant for tables to fall back on outside the range, which the program does
// not use: the format reference says to leave them blank, so they are kept as written (0 for a blank field), may hold
// any integer and name no card.
//
// A pair with both fields blank is padding (a line that stops short of its end) and is not kept, as for a TABLED1.
// The ITID is unique among the cards of the four multi-dimensional tables (IdSpace::PerCardType: the cards of the
// category MultiDimensionalTable, that the SOL 400 materials and a CORD3G name).
struct TableMultiVariable0Card
{
    base::EntityId id = 0;
    int kind = 0; // required, greater than zero
    int extrapolation = TABLE_DEFAULT_EXTRAPOLATION;
    base::EntityId belowRangeTableId = 0;
    base::EntityId aboveRangeTableId = 0;
    int smoothing = 0;
    std::vector<double> abscissa;
    std::vector<double> ordinate;
};

} // namespace oor::model
