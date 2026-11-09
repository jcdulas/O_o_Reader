#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// CFLAG of an unbalanced load: what the corrections of the transient analysis take into account.
enum class UnbalanceCorrection : std::uint8_t
{
    None,  // NONE (the default)
    Mass,  // MASS: the change of the total mass
    Speed, // SPEED: the change of the rotor spin rate
    Both   // BOTH
};

// One UNBALNC card: a rotating unbalance load of a rotordynamic transient or frequency response analysis (selected by
// the DLOAD command for a frequency response and the RGYRO command for a transient one; several cards may share a SID,
// and the set is one of the dynamic load sets).
//
// Card layout:
//   UNBALNC  SID     MASS    GRID     X1    X2    X3
//            ROFFSET THETA   ZOFFSET  T/FON T/FOFF CFLAG
//            UFT1    UFT2    UFT3     UFR1  UFR2  UFR3
//            MCT1    MCT2    MCT3     MCR1  MCR2  MCR3
//            SCR1    SCR2    SCR3
// MASS, ROFFSET and ZOFFSET are a real (a constant) or an integer (the ID of a TABLEDi that gives the value as a
// function of time or frequency): each lives in two members, `...TableId` and the real, at most one of them set. A
// blank ROFFSET is 1.0 and a blank ZOFFSET 0.0. The lines of the EPOINTs that output the forces are optional; the
// points are kept in the order UFT1..3, UFR1..3, MCT1..3, MCR1..3, SCR1..3 (0 is blank), and written line by line from
// the first one in use.
struct RotorUnbalanceLoadCard
{
    static constexpr int EXTRA_POINT_COUNT = 15;

    base::EntityId id = 0;
    base::EntityId massTableId = 0;
    double mass = 0.0;
    base::EntityId nodeId = 0;
    std::array<double, 3> direction{0.0, 0.0, 0.0}; // X1, X2, X3: the vector from GRID that defines the angle
    base::EntityId radialOffsetTableId = 0;
    double radialOffset = 1.0;
    double angularPosition = 0.0; // the angular position of the mass, in degrees
    base::EntityId axialOffsetTableId = 0;
    double axialOffset = 0.0;
    double startTimeOrFrequency = 0.0;     // the start time or frequency
    double stopTimeOrFrequency = 999999.0; // the stop time or frequency
    UnbalanceCorrection correctionFlag = UnbalanceCorrection::None;
    std::array<base::EntityId, EXTRA_POINT_COUNT> extraPoints{};
};

} // namespace oor::model
