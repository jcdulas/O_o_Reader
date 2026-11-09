#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <string>

namespace oor::model
{

// One SPLINEX card: a spline that a program supplied by the user evaluates, instead of one of the methods of the
// format. The card only says which meshes the spline joins and where its data is.
//
// Card layout: SPLINEX  EID     GROUP   DGCOMP  IGCOMP  DECOMP  IECOMP        USAGE
//                      AELIST  AEFACT  AELISTC
// Field 7 (the blank cell before USAGE) is not used and must be blank; the card has 11 fields. GROUP names the family
// of external splines the evaluating program handles (no default, 1 to 8 characters). The four COMP fields are the
// NAMES of AECOMP or AECOMPL cards, not IDs: DGCOMP and IGCOMP give the points of the dependent and the independent
// mesh (for an aero-to-structure spline "dependent" is the aerodynamic side), DECOMP and IECOMP their elements. A name
// is kept as written and matched without regard to the case, like every label, and a blank name is an empty string; no
// reference is visited, because a rule between cards (every name must be the name of an AECOMP or AECOMPL) compares
// them. When a mesh is structural its grid component may be left blank and the points come from the connectivity of the
// element component; the format reference says both may not be blank, which is read here for each mesh: DGCOMP and
// DECOMP are not both blank, and IGCOMP and IECOMP are not both blank.
//
// AELIST (ReferenceTarget::AeroList) and AEFACT (ReferenceTarget::AeroFactor) are the integer and the real data the
// program reads, and AELISTC is the AELISTC card of its character data (a plain ID: there is no reference target for
// it, so nothing visits it). The solver never looks inside them. The format reference gives the three no default; they
// are optional here (0 is blank), since a program that needs no real data has nothing to name.
struct AeroUserSplineCard
{
    base::EntityId id = 0;
    std::string groupName;
    std::string dependentNodeComponent;
    std::string independentNodeComponent;
    std::string dependentElemComponent;
    std::string independentElemComponent;
    SplineUsage usage = SplineUsage::Both;
    base::EntityId aeroBoxListId = 0;
    base::EntityId factorListId = 0;
    base::EntityId labelListId = 0;
};

} // namespace oor::model
