#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <cstdint>

namespace oor::model
{

// One FBALOAD card: the scale factor of a dynamic load on one degree of freedom of an FRF component, in an FRF based
// assembly (FBA). It plays the part of a DAREA card there (the program turns each into an equivalent DAREA),
// and the EXCITEID field of an RLOAD1, an RLOAD2 and an ACSRCE names it by its SID.
//
// Card layout: FBALOAD  SID  COMPID/COMPNAME  PNTID  C  A
// SID is a set ID shared by the cards of the set, as that of a DAREA. The component is the ID of an FRFCOMP or its name
// (a whole number is an ID, anything else a name). The format marks COMPNAME optional on this card only, but its
// layout has one COMPID/COMPNAME field and its description of COMPID says nothing of a default, so the card reads
// both forms like the other FRF cards and requires one of the two. PNTID is a grid or scalar point of that component's
// own model, a plain number never looked up. C is one component: 1 to 6 for a grid point, 0 or blank for a scalar
// point. A is the scale factor, any real. Unlike a DAREA, a card holds one group only.
struct FrfAssemblyLoadScaleCard
{
    base::EntityId id = 0;
    FrfComponentRef frfComponent;
    base::EntityId pointId = 0;
    std::int32_t components = 0;
    double scaleFactor = 0.0;

    [[nodiscard]] bool operator==(const FrfAssemblyLoadScaleCard&) const noexcept = default;
};

} // namespace oor::model
