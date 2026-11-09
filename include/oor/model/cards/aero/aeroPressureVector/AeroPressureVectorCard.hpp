#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One AEPRESS card: a vector of pressure per unit dynamic pressure that goes with one control vector of the aerodynamic
// configuration. The pressure is a matrix given by name, and the program turns it into forces on the aerodynamic grid
// for the nonlinear static aeroelastic trim. The AECONFIG the card belongs to is chosen by the Case Control,
// which is not part of the card.
//
// Card layout: AEPRESS  MACH  SYMXZ  SYMXY  UXID  DMIJ  DMIJI             fields 1 to 6
// The layout and the rules are the ones of the AEDW (the format reference describes DMIJ here as the pressure per unit
// dynamic pressure and not the downwash): MACH is zero or more and not 1.0 (`checkForceMach`); SYMXZ and SYMXY are one
// of SYMM, ASYMM and ANTI (`FORCE_SYMMETRY_WORDS`) and, with no default or blank in the format reference, both are
// required, as are UXID (the UXVEC of the control parameters) and DMIJ (the name of a DMI or DMIJ matrix). DMIJI, the
// name of the DMIJI matrix of the CAERO2 interference elements, is needed only when those elements have a nonzero
// downwash and may be blank.
//
// The matrix names are names, not references: they are kept as written, and a check of the model matches them with the
// names of the matrix cards without regard to the case.
struct AeroPressureVectorCard
{
    double machNumber = 0.0;
    ForceSymmetry symmetricXz = ForceSymmetry::Symmetric;
    ForceSymmetry symmetricXy = ForceSymmetry::Symmetric;
    base::EntityId controlVectorId = 0;                 // a UXVEC
    std::string matrixName;                             // the name of a DMI or DMIJ matrix
    std::optional<std::string> interpolationMatrixName; // the name of a DMIJI matrix, none when blank
};

} // namespace oor::model
