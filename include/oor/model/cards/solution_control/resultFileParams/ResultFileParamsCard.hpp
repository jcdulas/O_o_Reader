#pragma once

#include "oor/model/cards/solution_control/resultFileParams/Hdf5Parameters.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

// One HDF5OUT card: values for the parameters of the HDF5 output (the precision of the file, its compression, what is
// written to it); the eleven that the format lists are in `Hdf5Parameters.hpp`.
//
// Card layout:  HDF5OUT  PARAM1  VAL1  PARAM2  VAL2  ...  PARAMi  VALi          fields 1 to 8, then the next lines
// A pair is a name and a value in the next field, four to a line. A card may have an integer in the first field before
// the pairs, which the layout does not have and the format does not explain (it looks like the module number of an
// H5OUT carried over); the card reads an integer there as `number` and puts the
// pairs from the next field, and writes it back where it was. A name the table does not list is kept with its value as
// written (a later release may add names); a listed name takes the values the table gives it, whose defaults apply when
// no card gives the parameter. The card names no other card and has no ID (IdSpace::None, category Other: more than one
// is not refused, since the format does not ask for one only).
struct ResultFileParamsCard
{
    std::optional<int> number; // the integer before the first pair of the format's second example, kept as written
    std::vector<Hdf5Parameter> parameters;
};

} // namespace oor::model
