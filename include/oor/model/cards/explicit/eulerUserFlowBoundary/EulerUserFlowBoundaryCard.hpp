#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/BoundaryBox.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// One FLOWUDS card (SOL 700): a flow boundary of an Euler mesh that a user subroutine computes. The FMS statement
// CONNECT SERVICE gives the group a name and the library of routines it holds. No other entry of the format reference
// has this name.
//
// Card layout: FLOWUDS  LID  GROUP  UNAME  BCID  MESH  DIR            fields 1 to 6
//             +        XMIN  XMAX  YMIN  YMAX  ZMIN  ZMAX           fields 9 to 14
// LID is the number of a set of flow boundaries: several FLOWUDS cards (and FLOW cards) may share it, and a TLOAD1
// names it as its EXCITEID. GROUP is the group name of the CONNECT SERVICE statement (required, at most eight
// characters) and UNAME the name of the user subroutine, EXFLOW when blank (the standard user defined flow boundary,
// the only routine the format lists). The faces are chosen as for the other flow cards: BCID (a set of BCSEG cards),
// else DIR, else the box. The format also calls LID unique, which does not fit a number that several cards share; the
// card treats it as a shared set number. The names are kept in capitals.
struct EulerUserFlowBoundaryCard
{
    static constexpr std::string_view DEFAULT_ROUTINE_NAME = "ExternalFlow";

    base::EntityId setId = 0; // the set of flow boundaries, shared by the cards of the set
    std::string groupName;
    std::string userRoutineName{DEFAULT_ROUTINE_NAME};
    base::EntityId contactBodyId = 0; // a set of BCSEG cards, 0 when blank
    base::EntityId meshId = 0;        // an Euler MESH, 0 when blank
    std::optional<BoundaryFace> meshFace;
    BoundaryBox box;
};

} // namespace oor::model
