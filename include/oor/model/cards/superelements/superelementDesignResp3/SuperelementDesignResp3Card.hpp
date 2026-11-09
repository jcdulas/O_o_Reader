#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/superelements/SuperelementGroups.hpp"
#include "oor/model/cards/superelements/superelementDesignResp2/SuperelementDesignResp2Card.hpp"

#include <array>
#include <string>
#include <vector>

namespace oor::model
{

// The groups of an SEDRSP3: those of an SEDRSP2 and USRDATA, the text of the constants passed to the routine.
inline constexpr std::array<SuperelementGroupKeyword, 11> SUPERELEMENT_DESIGN_RESP3_GROUPS{{
    {"DesignVar", SuperelementGroupContent::Pairs, ReferenceTarget::DesignVariable, "designVarIds"},
    {"DesignConstants", SuperelementGroupContent::Labels, ReferenceTarget::Other, "constantLabels"},
    {"DesignResp1", SuperelementGroupContent::Pairs, ReferenceTarget::DesignResponse, "responseIds"},
    {"DesignNodes", SuperelementGroupContent::Nodes, ReferenceTarget::Node, "nodeIds"},
    {"DesignPropRelation1", SuperelementGroupContent::Pairs, ReferenceTarget::PropertyRelation, "propRelationIds"},
    {"DesignElemRelation1", SuperelementGroupContent::Pairs, ReferenceTarget::ConnectivityRelation, "elemRelationIds"},
    {"DesignMatRelation1", SuperelementGroupContent::Pairs, ReferenceTarget::MaterialRelation, "matRelationIds"},
    {"DesignPropRelation2", SuperelementGroupContent::Pairs, ReferenceTarget::PropertyRelation, "propRelation2Ids"},
    {"DesignElemRelation2", SuperelementGroupContent::Pairs, ReferenceTarget::ConnectivityRelation, "elemRelation2Ids"},
    {"DesignMatRelation2", SuperelementGroupContent::Pairs, ReferenceTarget::MaterialRelation, "matRelation2Ids"},
    {"UserData", SuperelementGroupContent::Text, ReferenceTarget::Other, "userData"},
}};

// One SEDRSP3 card: an external response, computed by a routine of the user from design variables, constants and
// responses of several part superelements, as a DRESP3 does for one model.
//
// Card layout: SEDRSP3  ID  LABEL  GROUP  TYPE  REGION                                 fields 1 to 5
//                      DESVAR  DVSEID1  DVID1 ...                                      the lines after, from field 9:
//                      the DTABLE ...  DRESP1 ...  DNODE ...  DVPREL1 ...  ...  USRDATA text   groups of SEDRSP2 and
//                      USRDATA
// Apart from the part superelement that leads each item, the card follows the rules of the DRESP3, so the
// fields of the first line are those of DesignResp3Card: GROUP names the group of external responses and TYPE the
// response in it. An item may name superelement 0, the residual structure. The card belongs in the main Bulk Data and
// is ignored after a BEGIN SUPER=n with n above 0; the DRESP2 flag is not supported. Its ID is a
// response number, one of the numbers of DRESP1, DRESP2 and DRESP3 (category DesignResponse, one ID space per card
// type).
struct SuperelementDesignResp3Card
{
    base::EntityId id = 0;
    std::string entryLabel;
    std::string groupName;                 // upper case
    std::string kind;                      // upper case
    base::EntityId regionId = 0;           // 0: blank
    std::vector<SuperelementGroup> groups; // in the order written

    [[nodiscard]] bool operator==(const SuperelementDesignResp3Card&) const noexcept = default;
};

} // namespace oor::model
