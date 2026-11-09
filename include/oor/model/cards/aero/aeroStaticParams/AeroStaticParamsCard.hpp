#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// The AEROS card: the basic parameters of a static aeroelastic analysis (trim, divergence, stability derivatives). A
// model has one; the table reports a second one when a deck is read.
//
// Card layout: AEROS  ACSID  RCSID  REFC  REFB  REFS  SYMXZ  SYMXY
// ACSID is the rectangular coordinate system of the aerodynamics (+x is the flow); RCSID the one in which the rigid
// body motions of the trim variables (the AESTAT degrees of freedom) are expressed. Both are 0, the basic system, when
// blank. That they exist and are rectangular is a rule between cards, not checked here. REFC, REFB and REFS are the
// reference chord, span and area, required and positive. The span is the full span even for a half model, while the
// area of a half model is the half area: the card cannot know which model it belongs to.
//
// SYMXZ and SYMXY are the symmetry keys of the x-z and x-y planes (see `SymmetryKey.hpp` for their signs); the Case
// Control AESYMXZ and AESYMXY override them.
struct AeroStaticParamsCard
{
    base::EntityId aeroFrameId = 0;
    base::EntityId referenceFrameId = 0;
    double referenceChord = 0.0;
    double referenceSpan = 0.0;
    double referenceArea = 0.0;
    std::int32_t symmetricXz = 0;
    std::int32_t symmetricXy = 0;
};

} // namespace oor::model
