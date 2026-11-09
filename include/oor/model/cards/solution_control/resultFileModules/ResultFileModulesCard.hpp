#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// A module and the UDNAME that names the HDF5 file of its output (MDIDi and UDIDi).
struct ResultFileModulesModule
{
    int moduleId = 0;              // the module number, 0 for the residual structure (the main Bulk Data)
    base::EntityId userNameId = 0; // the UDNAME of the file

    [[nodiscard]] bool operator==(const ResultFileModulesModule&) const noexcept = default;
};

// One HDF5DA card: the list of the modules whose output goes to a dedicated or appended HDF5 file, and the file of each
// (a module BEGIN MODULE keeps its own Bulk Data, a part of the deck).
//
// Card layout:
//   HDF5DA  ID  CONFIG                                                        fields 1 and 2
//           MDID0  UDID0  MDID1  UDID1  MDID2  UDID2  MDID3  UDID3            four pairs on each line, from field 2
// The pairs are on the continuation lines, from their field 2, and fields 3 to 9 of the first line are empty. CONFIG is
// the UDNAME that names the assembly configuration (at most 64 characters long: a rule of the file name, not of this
// card), 0 or blank for none. Every module of the model, the residual structure included, has a pair, so at least one
// pair is needed, and a module has one pair: a repeated MDID is refused. The UDIDi are not unique (two modules may
// write to one file), and need not exist as a module here (a module is text in this model), so the MDIDi are plain
// numbers and the UDIDi and CONFIG are UserDefinedName references.
//
// The ID is unique among the HDF5DA cards (IdSpace::PerCardType, category Other: the Case Control HDF5DA command names
// it).
struct ResultFileModulesCard
{
    base::EntityId id = 0;
    base::EntityId configuration = 0; // a UDNAME, 0 when blank
    std::vector<ResultFileModulesModule> modules;
};

} // namespace oor::model
