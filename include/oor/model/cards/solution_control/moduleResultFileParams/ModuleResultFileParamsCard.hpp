#pragma once

#include "oor/model/cards/solution_control/resultFileParams/Hdf5Parameters.hpp"

#include <vector>

namespace oor::model
{

// One H5OUT card: values for the HDF5 output parameters of one module, which override those of the HDF5OUT card for
// that module (the PRCISION, CMPRMTHD and LEVEL of this card are ignored). The parameters and
// their values are those of the HDF5OUT card, which `Hdf5Parameters.hpp` has.
//
// Card layout:  H5OUT  MDID                                                       fields 1 and 2 (the module, 0 is the
// main Bulk Data)
//                     PARAM1  VAL1  PARAM2  VAL2  ...                            fields 2 to 9 of the continuation
//                     lines
// The module number is alone on the first line and the pairs are on the next ones, from their field 2, four pairs to a
// line. The reader takes the pairs from there only: a value in
// fields 3 to 9 of the first line is refused. A name the table lists takes the values the table gives it; a name it
// does not list is kept with its value as written, as for the HDF5OUT.
//
// The module number is a plain integer (a module is a part of the deck, which no card defines), 0 or more. The card has
// no ID: a deck may repeat a module (the format does not forbid it), so it is IdSpace::None, category Other, and the
// module is a field like any other.
struct ModuleResultFileParamsCard
{
    int moduleId = 0;
    std::vector<Hdf5Parameter> parameters;
};

} // namespace oor::model
