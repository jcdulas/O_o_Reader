#pragma once

#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One BCBMRAD card: the equivalent radius of beams for the node-to-segment beam-to-beam contact (SOLs 101 and 400): the
// radius of the beam elements or bodies it lists, or the default radius of all beam elements. The radius of an element
// overrides the one of its body, which overrides the default.
//
// Card layout: BCBMRAD  RADIUS  TYPE  ID1  ID2  ID3 ...    with ranges   ID1  THRU  ID2  BY  N
// over as many continuation lines as needed. TYPE is EID (the IDs are beam elements: CROD, CBAR, CBEAM, CBEAM3; the
// default), BODY (the IDs are BCBODY entries) or ALL (no ID: the default radius of every beam).
struct ContactBeamRadiusCard
{
    double radius = 0.0;           // the outer radius of a tube or round bar; for another section an equivalent radius
    std::string idKind = "ElemId"; // EID, BODY or ALL
    std::vector<ContactRun> ids;   // the beam elements or bodies; empty with ALL
};

} // namespace oor::model
