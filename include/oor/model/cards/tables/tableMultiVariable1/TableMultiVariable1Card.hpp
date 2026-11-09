#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/tables/TableClosureSupport.hpp"

#include <cstddef>
#include <vector>

namespace oor::model
{

// One independent variable of a TABL3D1: what it is, how the table behaves past its data, and the values it takes.
struct TableMultiVariable1Variable
{
    int kind = 0; // KINDi, greater than zero
    int extrapolation = TABLE_DEFAULT_EXTRAPOLATION;
    base::EntityId belowRangeTableId = 0; // ITIDSi and ITIDBi: not used by the program, kept as written
    base::EntityId aboveRangeTableId = 0;
    int smoothing = 0;
    std::vector<double> x; // the NWi values of the variable (NWi is the number of values)
};

// One TABL3D1 card (SOL 400): the second form of the TABL3D entry, a quantity that is a function of one to four
// variables (strain, temperature, strain rate, ...) whose values are on a grid. The data are entered one row at a time:
// the values of each variable, then the values of the function on the grid they make.
//
// Card layout (the form whose name field reads TABL3D1):
//   TABL3D1  ITID    NV      KIND1    KIND2    KIND3    KIND4    NW1      NW2        fields 1 to 8
//            NW3     NW4     EXTRP1   EXTRP2   EXTRP3   EXTRP4   (blank)  (blank)    9 to 16
//            ITIDS1  ITIDB1  ITIDS2   ITIDB2   ITIDS3   ITIDB3   ITIDS4   ITIDB4     17 to 24
//            SM1     SM2     SM3      SM4      (blank)  ...                         25 to 32
//            X11     X12     X13      -etc.-                                         the NW1 values of variable 1
//            X21     X22     -etc.-                                                  the NW2 values of variable 2
//            (the X lines of the variables 3 and 4 when NW3 and NW4 are above zero)
//            Y1      Y2      Y3       -etc.-                                         the values of the function
// Each list of X values starts on a line of its own (field 33 for the first, then the next line boundary), and so do
// the Y values: the card has blank fields between the end of a list and the next line, and no other field after its
// last value. The format gives no ENDT. Eight values to a line.
//
// The card is a function of the first NV variables, every one of them with NWi greater than zero: `variables` holds the
// NV of them (1 to 4) and the number of values of each is the size of its `x`. The fields of a variable beyond NV
// (KIND, NW, EXTRP, ITIDS, ITIDB, SM) are blank; EXTRP and SM may hold their defaults, 2 and 0, there, which means the
// same thing. The Y values are one per point of the grid, so there are NW1 x NW2 x ... of them. The format does not
// draw the order in which a row of the grid follows the others and the card does not evaluate the table: `y` keeps the
// order of the card. KIND, EXTRP, SM, ITIDS and ITIDB have the meaning they have in a TABL3D0 (see its card).
//
// The ITID is unique among the cards of the four multi-dimensional tables (IdSpace::PerCardType).
struct TableMultiVariable1Card
{
    static constexpr std::size_t MAX_VARIABLES = 4;

    base::EntityId id = 0;
    std::vector<TableMultiVariable1Variable> variables; // NV = size(), 1 to 4
    std::vector<double> ordinate;
};

} // namespace oor::model
