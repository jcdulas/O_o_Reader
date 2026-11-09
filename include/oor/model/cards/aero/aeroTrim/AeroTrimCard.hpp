#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// A trim variable the card fixes: the label of an AESTAT, AESURF or AEPARM and the value it takes.
struct AeroTrimVariable
{
    std::string label;
    double value = 0.0;

    [[nodiscard]] bool operator==(const AeroTrimVariable&) const noexcept = default;
};

// One TRIM card: the flight condition of a static aeroelastic trim and the trim variables that are fixed in it.
//
// Card layout: TRIM  SID  MACH  Q   LABEL1  UX1  LABEL2  UX2  AEQR
//                   LABEL3  UX3  LABEL4  UX4  -etc.-
// The pairs of a label and a value are in fields 4 and 5, 6 and 7, then (AEQR having field 8) from field 9 on, four to
// a continuation line. A variable that no pair names is free: the trim solves for it. The Case Control TRIM selects the
// card by its SID (shared with the TRIM2 cards); the labels name an AESTAT, an AESURF or an AEPARM by their LABEL (not
// by their ID), so a renumbering does not touch them, and are compared without regard to the case.
//
// MACH is not negative and not 1.0 (the Doublet-Lattice theory below 1.0, ZONA51 above it), Q is positive. AEQR is 1.0
// for the elastic trim (blank), 0.0 for the rigid trim; the values between have no physical meaning but are allowed.
struct AeroTrimCard
{
    base::EntityId id = 0;
    double machNumber = 0.0;
    double dynamicPressure = 0.0;
    double equivalentAspectRatio = 1.0;
    std::vector<AeroTrimVariable> variables;
};

} // namespace oor::model
