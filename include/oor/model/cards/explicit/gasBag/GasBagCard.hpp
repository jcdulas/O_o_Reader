#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// TRIGGER (field 3): the offset of the time dependent parameters is a time, and TIME is the only word. A blank TRIGGER
// is TIME.
enum class GasBagTrigger : std::uint8_t
{
    Time
};
inline constexpr std::array<std::string_view, 1> GAS_BAG_TRIGGER_WORDS{"Time"};

// CPGAS and CPGASV (fields 25 and 26): the specific heat at constant pressure of the gas is a constant, the only
// variation the format reference lists. A blank CPGAS is CONSTANT; CPGASV has no default (the layout's 0.0 is a value
// the card refuses, so a blank CPGASV is reported by the card).
inline constexpr SourcedValueLayout GAS_BAG_SPECIFIC_HEAT_LAYOUT{
    25, 26, "specificHeatSource", "specificHeatPressure", sourceBit(ValueSource::Constant), ValueSource::Constant, 0.0};

// One GBAG card (SOL 700): a gas bag, the uniform pressure within a closed volume that the surface BSID encloses. The
// pressure follows from the gas inside (its initial state, the inflators that fill it and the holes and porous parts
// that let it out) and from the environment around the bag. COUPLE has the same fields for a bag that is meshed with
// Euler elements.
//
// Card layout, five lines of the small format (fields 9 to 24 are blank):
//   GBAG  GID  BSID  TRIGGER  TRIGGERV  PORID  INFID  HTRID  INTID                  fields 1 to 8
//   CPGAS  CPGASV  RGAS  PENV  (blank)  REVERSE  CHECK  PINIT                        fields 25 to 32
//   TINIT  TENV                                                                     fields 33 and 34
// (the layout also spells CPGASV as GPGASV, a misprint; the card reads CPGASV).
// TRIGGERV is marked required, but a gas bag with no trigger has no use for it: a blank TRIGGERV is no offset. PINIT
// defaults to PENV and TENV to TINIT, so the card keeps both resolved and always writes them. The format reference says
// that REVERSE = ON switches CHECK on whatever its value; the card keeps the CHECK it is given.
//
// The IDs: GID numbers the bag (unique among the GBAG cards). BSID names the BSURF, BCBOX, BCPROP, BCMATL or BCSEG
// of the surface. PORID is the number of a set of LEAKAGE cards (a set ID), INFID the number of a set of ABINFL cards
// (a set ID) and INTID the number of an INITGAS. HTRID is the number of a set of HEATLOS cards (a set ID too: the
// target is ReferenceTarget::HeatLossSet). A blank ID (0) is a bag without that part.
struct GasBagCard
{
    base::EntityId id = 0;
    base::EntityId surfaceId = 0;
    GasBagTrigger trigger = GasBagTrigger::Time;
    double triggerOffset = 0.0;        // the offset in time, 0.0 for none
    base::EntityId porosityId = 0;     // a set of LEAKAGE cards, 0 for no porosity
    base::EntityId inflatorId = 0;     // a set of ABINFL cards, 0 for no inflator
    base::EntityId heatTransferId = 0; // a set of HEATLOS cards, 0 for no heat transfer
    base::EntityId initialGasId = 0;   // an INITGAS, 0 for no initial gas composition
    SourcedValue specificHeat{ValueSource::Constant, 0.0, 0};
    double gasConstant = 0.0;         // the gas constant, greater than zero
    double environmentPressure = 0.0; // the pressure of the environment
    Switch reverse = Switch::On;
    Switch check = Switch::On;
    double initialPressure = 0.0;        // the initial pressure, PENV when the field is blank
    double initialTemperature = 0.0;     // the initial temperature, greater than zero
    double environmentTemperature = 0.0; // the temperature of the environment, TINIT when the field is blank
};

} // namespace oor::model
