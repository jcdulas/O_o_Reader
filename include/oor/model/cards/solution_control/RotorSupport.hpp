#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/solution_control/rotordynamicParams/RotordynamicParamsCard.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <array>
#include <cstdint>
#include <string_view>

// What the rotordynamic cards share (RSPINR, RSPINT, CAMPBLL and RGYRO): the
// unit of a speed and the visit of a rotor ID. Each helper moves one kind of value; every card still lays out, reads,
// writes and checks its own fields by hand.
namespace oor::model
{

// The words of a speed unit field, in the order of RotorSpeedUnit (the enumeration RGYRO already has): RPM is
// revolutions per minute, FREQ revolutions (cycles) per unit time.
inline constexpr std::array<std::string_view, 2> ROTOR_SPEED_UNIT_WORDS{"RevolutionsPerMinute", "Frequency"};

// Visits the ID of a rotor (ReferenceTarget::Rotor: a ROTORG, ROTOR, ROTORAX or ROTORSE) and stores what the visitor
// returns. The cards that say in which superelement the rotor is defined (ROTRSEID of an RSPINR, an RSPINT and an
// RGYRO, the REFROTSE and NFROTSEi of a REFRTSE) do not look it up when that number is not 0: the rotor is then a card
// of the part of the superelement, not of the residual structure the visitor walks. A blank field (0) is not visited.
inline void visitRotor(ReferenceVisitor& visitor, std::string_view field, base::EntityId& rotorId,
                       std::int32_t superelement)
{
    if (superelement == 0)
        visitId(visitor, field, ReferenceTarget::Rotor, rotorId);
}

} // namespace oor::model
