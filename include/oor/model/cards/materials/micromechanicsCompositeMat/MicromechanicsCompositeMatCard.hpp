#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One MATDIGI card: the material data of an advanced composite modelled by a micromechanics tool (SOL 400 only; SOL
// 400 uses the three fields of this card and ignores the others).
//
// Card layout:
//   MATDIGI  MID  UDID  RHO                                                      fields 1 to 3
// UDID is the ID of the UDNAME card that names the file of that tool: a UserDefinedName reference, so a renumbering of
// the UDNAME cards follows it. RHO, the density, is absent when blank and greater than
// zero otherwise. The material ID is in the space every material type shares.
struct MicromechanicsCompositeMatCard
{
    base::EntityId id = 0;
    base::EntityId userDefinedNameId = 0;
    std::optional<double> density;
};

} // namespace oor::model
