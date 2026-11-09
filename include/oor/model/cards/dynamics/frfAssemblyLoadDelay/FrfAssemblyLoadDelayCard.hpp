#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/dynamics/FrfComponentReference.hpp"

#include <cstdint>

namespace oor::model
{

// One FBADLAY card: the time delay of a dynamic load on one degree of freedom of an FRF component, in an FRF based
// assembly (FBA). It plays the part of a DELAY card there (the program turns each into an equivalent DELAY),
// and the DELAY field of an RLOAD1, an RLOAD2 and an ACSRCE names it by its SID.
//
// Card layout: FBADLAY  SID  COMPID/COMPNAME  PNTID  C  DELAY
// SID is a set ID shared by the cards of the set, as that of a DELAY. The component is the ID of an FRFCOMP or its name
// (a whole number is an ID, anything else a name); PNTID is a grid or scalar point of that component's own model, a
// plain number never looked up. C is one component: 1 to 6 for a grid point, 0 or blank for a scalar point. DELAY is
// the time delay, any real. Unlike a DELAY, a card holds one group only.
struct FrfAssemblyLoadDelayCard
{
    base::EntityId id = 0;
    FrfComponentRef frfComponent;
    base::EntityId pointId = 0;
    std::int32_t components = 0;
    double delay = 0.0;

    [[nodiscard]] bool operator==(const FrfAssemblyLoadDelayCard&) const noexcept = default;
};

} // namespace oor::model
