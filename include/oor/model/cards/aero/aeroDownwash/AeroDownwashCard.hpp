#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <optional>
#include <string>

namespace oor::model
{

// One AEDW card: the normal wash (downwash) loading that goes with one control vector of the aerodynamic configuration.
// The downwash is a matrix given by name, and the program turns it into forces on the aerodynamic grid for the
// nonlinear static aeroelastic trim. Which AECONFIG the card belongs to is decided by the Case Control, or by the BEGIN
// AECONFIG partition of the Bulk Data; neither is part of the card.
//
// Card layout: AEDW  MACH  SYMXZ  SYMXY  UXID  DMIJ  DMIJI                fields 1 to 6
// MACH is zero or more and not 1.0 (`checkForceMach`). SYMXZ and SYMXY are one of SYMM, ASYMM and ANTI, the symmetry of
// the force vector about the xz and the xy planes (`FORCE_SYMMETRY_WORDS`). The format reference gives neither a
// default nor a blank for them, so both are required, like UXID, the number of the UXVEC that holds the control
// parameters of the vector, and DMIJ, the name of the DMI or DMIJ matrix that gives the downwash. DMIJI is the name of
// a DMIJI matrix with the downwash of the interference elements of a CAERO2 body: it is needed only when those elements
// have a downwash, so it may be blank. The program expects an AEDW for each Mach number and symmetry the
// analysis uses.
//
// The two matrix names are names, not references: the library does not follow them (a check of the model matches DMIJ
// with the name of a DMI or DMIJ card and DMIJI with the name of a DMIJI card, without regard to the case). They are
// kept as written.
struct AeroDownwashCard
{
    double machNumber = 0.0;
    ForceSymmetry symmetricXz = ForceSymmetry::Symmetric;
    ForceSymmetry symmetricXy = ForceSymmetry::Symmetric;
    base::EntityId controlVectorId = 0;                 // a UXVEC
    std::string matrixName;                             // the name of a DMI or DMIJ matrix
    std::optional<std::string> interpolationMatrixName; // the name of a DMIJI matrix, none when blank
};

} // namespace oor::model
