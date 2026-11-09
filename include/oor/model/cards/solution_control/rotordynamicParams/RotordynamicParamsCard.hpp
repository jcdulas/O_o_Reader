#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// SYNCFLG of a rotordynamic analysis: the rotors turn with the reference rotor (SYNC) or at speeds of their own
// (ASYNC). A blank is allowed for a static analysis.
enum class RotorSynchronisation : std::uint8_t
{
    NotGiven, // blank
    Sync,     // SYNC
    Async     // ASYNC
};

// The unit of the speeds of an RGYRO card.
enum class RotorSpeedUnit : std::uint8_t
{
    Rpm, // RPM: revolutions per minute
    Freq // FREQ: revolutions (cycles) per unit time
};

// One RGYRO card: the parameters of a rotordynamic analysis (selected by RGYRO in Case Control; several cards may share
// a RID).
//
// Card layout:
//   RGYRO  RID  SYNCFLG  REFROTR  SPDUNIT  SPDLOW  SPDHIGH  SPEED  ROTRSEID
//          WR3WRL  WR4WRL  WRHWRL
// The reference rotor is REFROTR (defined in superelement ROTRSEID, 0 for the residual structure: only then is it a
// rotor of the model, a Rotor reference; a rotor of a superelement is a card of that part). SPDLOW and SPDHIGH
// are the speed range of a synchronous analysis, SPEED the speed of an asynchronous or static one; WR3WRL, WR4WRL and
// WRHWRL are the whirl frequencies of the rotor damping terms, in radians per unit time.
struct RotordynamicParamsCard
{
    base::EntityId id = 0;
    RotorSynchronisation synchronous = RotorSynchronisation::NotGiven;
    base::EntityId referenceRotorId = 0;
    RotorSpeedUnit speedUnit = RotorSpeedUnit::Rpm;
    double speedLow = 0.0;
    double speedHigh = 99999.0;
    double speed = 0.0;
    std::int32_t rotorSuperelementId = 0;
    double viscousDampingFrequency = 0.0;
    double structuralDampingFrequency = 0.0;
    double hybridDampingFrequency = 0.0;
};

} // namespace oor::model
