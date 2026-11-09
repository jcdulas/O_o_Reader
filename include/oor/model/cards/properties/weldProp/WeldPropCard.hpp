#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PWELD card: the property of a connector element (CWELD).
//
// Card layout:
//   PWELD  PID    MID  D   (blank)  (blank)  MSET   (blank)  TYPE
//          LDMIN  LDMAX
//
// MSET (ON or OFF, the default), LDMIN and LDMAX are active only with PARAM,OLDWELD,YES. TYPE is blank (a general
// connector) or SPOT (a spot weld connector). The fields 5, 6 and 8 are blank, except in the older layout the solver
// still reads, whose words in fields 5 and 6 (FF, CLAMP) are ignored (WeldPropFormat::read).
struct WeldPropCard
{
    static constexpr double DEFAULT_MIN_LENGTH_RATIO = 0.2;
    static constexpr double DEFAULT_MAX_LENGTH_RATIO = 5.0;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    double diameter = 0.0;             // diameter of the connector
    bool eliminateConstraints = false; // MSET = ON: the constraints of the connector are eliminated (dependent set)
    bool spot = false;                 // TYPE = SPOT
    double minLengthRatio = DEFAULT_MIN_LENGTH_RATIO; // smallest ratio of length to diameter for the stiffness
    double maxLengthRatio = DEFAULT_MAX_LENGTH_RATIO; // largest ratio of length to diameter for the stiffness
};

} // namespace oor::model
