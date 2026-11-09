#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// The AERO card: the basic aerodynamic parameters of the unsteady analyses (flutter, frequency and transient response
// to a gust). A model has one; the table reports a second one when a deck is read.
//
// Card layout: AERO  ACSID  VELOCITY  REFC  RHOREF  SYMXZ  SYMXY
// ACSID is the rectangular coordinate system of the aerodynamics, whose +x axis is the direction of the flow; 0, which
// a blank field also means, is the basic system. That the system exists and is rectangular is a rule between cards,
// not checked here. VELOCITY is the speed used to recover aerodynamic forces and to form
// the BOV parameter, and it is the one the GUST cards must repeat; a blank field leaves it undefined. REFC is the
// reference length of the reduced frequency and RHOREF the reference density, which is read in mass units (PARAM,WTMASS
// does not scale it). Both are required and positive.
//
// SYMXZ and SYMXY are the symmetry keys of the x-z and x-y planes (see `SymmetryKey.hpp` for their signs). They count
// only when the Case Control has neither AESYMXY nor AESYMXZ: the model, not the card, tells which wins.
struct AeroUnsteadyParamsCard
{
    base::EntityId aeroFrameId = 0;
    std::optional<double> velocity; // positive when given
    double referenceChord = 0.0;
    double referenceDensity = 0.0;
    std::int32_t symmetricXz = 0;
    std::int32_t symmetricXy = 0;
};

} // namespace oor::model
