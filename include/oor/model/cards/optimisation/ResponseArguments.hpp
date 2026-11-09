#pragma once

#include "oor/model/cards/optimisation/DesignGroups.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// The arguments of a DRESP2 or a DRESP3: groups of numbers, each introduced by a keyword on its own line, that say what
// the equation (or the routine) takes as its arguments. The keywords must come in the order below, which is also the
// order of the arguments: the design variables, the constants of the DTABLE
// cards, the DRESP1 responses, the displacements of grid points, the relations, the other DRESP2 responses.
//
//   DESVAR  DVID1 ...      design variables             DVPREL2  DPI2P1 ...   property relations (equations)
//   DTABLE  LABL1 ...      constants (by label)         DVCREL2  DCI2C1 ...   connectivity relations (equations)
//   DRESP1  NR1 ...        DRESP1 responses             DVMREL2  DMI2M1 ...   material relations (equations)
//   DNODE   G1 C1 ...      grid point and component     DRESP2   NRR1 ...     DRESP2 responses
//   DVPREL1 DPIP1 ...      property relations           DVLREL1  DLIL1 ...    load relations (SOL 200 statics)
//   DVCREL1 DCIC1 ...      connectivity relations       USRDATA  text         user data (DRESP3 only)
//   DVMREL1 DMIM1 ...      material relations
inline constexpr std::array<GroupKeyword, 12> DESIGN_RESP2_GROUPS{{
    {"DesignVar", GroupContent::Integers, "designVarIds"},
    {"DesignConstants", GroupContent::Words, "constantLabels"},
    {"DesignResp1", GroupContent::Integers, "responseIds"},
    {"DesignNodes", GroupContent::Components, "nodeIds"},
    {"DesignPropRelation1", GroupContent::Integers, "propRelationIds"},
    {"DesignElemRelation1", GroupContent::Integers, "elemRelationIds"},
    {"DesignMatRelation1", GroupContent::Integers, "matRelationIds"},
    {"DesignPropRelation2", GroupContent::Integers, "propRelation2Ids"},
    {"DesignElemRelation2", GroupContent::Integers, "elemRelation2Ids"},
    {"DesignMatRelation2", GroupContent::Integers, "matRelation2Ids"},
    {"DesignResp2", GroupContent::Integers, "combinedResponseIds"},
    {"DesignLoadRelation1", GroupContent::Integers, "loadRelationIds"},
}};

// A DRESP3 has the same groups and the text of USRDATA after them.
inline constexpr std::array<GroupKeyword, 13> DESIGN_RESP3_GROUPS{{
    {"DesignVar", GroupContent::Integers, "designVarIds"},
    {"DesignConstants", GroupContent::Words, "constantLabels"},
    {"DesignResp1", GroupContent::Integers, "responseIds"},
    {"DesignNodes", GroupContent::Components, "nodeIds"},
    {"DesignPropRelation1", GroupContent::Integers, "propRelationIds"},
    {"DesignElemRelation1", GroupContent::Integers, "elemRelationIds"},
    {"DesignMatRelation1", GroupContent::Integers, "matRelationIds"},
    {"DesignPropRelation2", GroupContent::Integers, "propRelation2Ids"},
    {"DesignElemRelation2", GroupContent::Integers, "elemRelation2Ids"},
    {"DesignMatRelation2", GroupContent::Integers, "matRelation2Ids"},
    {"DesignResp2", GroupContent::Integers, "combinedResponseIds"},
    {"DesignLoadRelation1", GroupContent::Integers, "loadRelationIds"},
    {"UserData", GroupContent::Text, "userText"},
}};

// The problem with the groups of a response card, if there is one: what checkDesignGroups finds, no argument at all (at
// least one group of values must exist), a component other than 1 to 3 in DNODE.
[[nodiscard]] std::optional<CardProblem> checkResponseArguments(const std::vector<DesignGroup>& groups,
                                                                std::span<const GroupKeyword> keywords);

// Visits what the groups name: the design variables, the DRESP1 and DRESP2 responses, the relations and the grid points
// of DNODE (the load relations are not typed, and the constants are named by label).
void visitResponseArguments(ReferenceVisitor& visitor, std::vector<DesignGroup>& groups);

} // namespace oor::model
