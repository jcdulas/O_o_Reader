#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <cstdint>

namespace oor::model
{

// One FBAPHAS card: the phase lead of a dynamic load on one degree of freedom of an FRF component, in an FRF based
// assembly (FBA). It plays the part of a DPHASE card there (the program turns each into an equivalent
// DPHASE), and the DPHASE field of an RLOAD1, an RLOAD2 and an ACSRCE names it by its SID.
//
// Card layout: FBAPHAS  SID  COMPID/COMPNAME  PNTID  C  PHASE
// SID is a set ID shared by the cards of the set, as that of a DPHASE. The component is the ID of an FRFCOMP or its
// name (a whole number is an ID, anything else a name); PNTID is a grid or scalar point of that component's own model,
// a plain number never looked up (the format's description names it PNTIDi, a leftover from the DPHASE card that has
// several groups: this card holds one). C is one component: 1 to 6 for a grid point, 0 or blank for a scalar point.
// PHASE is the phase lead, any real.
struct FrfAssemblyLoadPhaseCard
{
    base::EntityId id = 0;
    FrfComponentRef frfComponent;
    base::EntityId pointId = 0;
    std::int32_t components = 0;
    double phase = 0.0;

    [[nodiscard]] bool operator==(const FrfAssemblyLoadPhaseCard&) const noexcept = default;
};

} // namespace oor::model
