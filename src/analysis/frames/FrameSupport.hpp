#pragma once

#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/properties/solidProp/SolidPropTable.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <optional>
#include <vector>

// What the frame code of the analysis layer shares and no one else needs: the property of an element, the CORDM of a
// solid property, the IDs a table talks about.

namespace oor::analysis::detail
{

// The PID of the element at `location` (shells, solids, rods, tubes, bars, beams, bushes, shear panels); nullopt for
// another card type.
[[nodiscard]] std::optional<base::EntityId> elementPropertyId(const model::Model& model, model::CardLocation location);

// The CORDM of the PSOLID with this ID; nullopt when the property is not a PSOLID or does not exist.
[[nodiscard]] std::optional<base::EntityId> solidCordm(const model::Model& model, base::EntityId pid);

// The element IDs of the rows of a table, one step and one component decoded (what the rows have in common).
[[nodiscard]] std::vector<std::int32_t> elementIdsOf(const results::ResultTable& table,
                                                     const results::DecodeContext& context);

} // namespace oor::analysis::detail
